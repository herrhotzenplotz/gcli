/*
 * Copyright 2021 Nico Sonack <nsonack@herrhotzenplotz.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <gcli/config.h>
#include <gcli/curl.h>
#include <gcli/github/config.h>
#include <gcli/github/releases.h>
#include <gcli/json_util.h>
#include <pdjson/pdjson.h>

#include <assert.h>

static void
github_parse_release(struct json_stream *input, gcli_release *out)
{
	enum json_type  next = JSON_NULL;
	const char     *key;

	if ((next = json_next(input)) != JSON_OBJECT)
		errx(1, "Expected Release Object");

	while ((next = json_next(input)) == JSON_STRING) {
		size_t len;
		key = json_get_string(input, &len);

		if (strncmp("name", key, len) == 0) {
			out->name = get_sv(input);
		} else if (strncmp("id", key, len) == 0) {
			int id = get_int(input);
			out->id = sn_sv_fmt("%d", id);
		} else if (strncmp("body", key, len) == 0) {
			out->body = get_sv(input);
		} else if (strncmp("tarball_url", key, len) == 0) {
			out->tarball_url = get_sv(input);
		} else if (strncmp("author", key, len) == 0) {
			out->author = get_user_sv(input);
		} else if (strncmp("created_at", key, len) == 0) {
			out->date = get_sv(input);
		} else if (strncmp("draft", key, len) == 0) {
			out->draft = get_bool(input);
		} else if (strncmp("prerelease", key, len) == 0) {
			out->prerelease = get_bool(input);
		} else if (strncmp("upload_url", key, len) == 0) {
			out->upload_url = get_sv(input);
		} else if (strncmp("html_url", key, len) == 0) {
			out->html_url = get_sv(input);
		} else {
			SKIP_OBJECT_VALUE(input);
		}
	}

	if (next != JSON_OBJECT_END)
		errx(1, "Unclosed Release Object");
}

int
github_get_releases(
	const char     *owner,
	const char     *repo,
	int             max,
	gcli_release **out)
{
	char               *url      = NULL;
	char               *e_owner  = NULL;
	char               *e_repo   = NULL;
	char               *next_url = NULL;
	gcli_fetch_buffer  buffer   = {0};
	struct json_stream  stream   = {0};
	enum json_type      next     = JSON_NULL;
	int                 size     = 0;

	*out = NULL;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/releases",
		gcli_get_apibase(),
		e_owner, e_repo);

	do {
		gcli_fetch(url, &next_url, &buffer);

		json_open_buffer(&stream, buffer.data, buffer.length);
		json_set_streaming(&stream, 1);

		if ((next = json_next(&stream)) != JSON_ARRAY)
			errx(1, "Expected array of releses");

		while ((next = json_peek(&stream)) == JSON_OBJECT) {
			*out = realloc(*out, sizeof(**out) * (size + 1));
			gcli_release *it = &(*out)[size++];
			github_parse_release(&stream, it);

			if (size == max)
				break;
		}

		json_close(&stream);
		free(url);
		free(buffer.data);
	} while ((url = next_url) && (max == -1 || size < max));

	free(next_url);
	free(e_owner);
	free(e_repo);

	return size;
}

static void
github_parse_single_release(gcli_fetch_buffer buffer, gcli_release *out)
{
	struct json_stream stream  = {0};

	json_open_buffer(&stream, buffer.data, buffer.length);
	json_set_streaming(&stream, 1);
	github_parse_release(&stream, out);
	json_close(&stream);
}

static char *
github_get_upload_url(gcli_release *it)
{
	char *delim = strchr(it->upload_url.data, '{');
	if (delim == NULL)
		errx(1, "GitHub API returned an invalid upload url");

	size_t len = delim - it->upload_url.data;
	return sn_strndup(it->upload_url.data, len);
}

static void
github_upload_release_asset(const char *url, gcli_release_asset asset)
{
	char               *req           = NULL;
	sn_sv               file_content  = {0};
	gcli_fetch_buffer  buffer        = {0};

	file_content.length = sn_mmap_file(asset.path, (void **)&file_content.data);
	if (file_content.length == 0)
		errx(1, "Trying to upload an empty file");

	/* TODO: URL escape this */
	req = sn_asprintf("%s?name=%s", url, asset.name);

	gcli_post_upload(
		req,
		"application/octet-stream", /* HACK */
		file_content.data,
		file_content.length,
		&buffer);

	free(req);
	free(buffer.data);
}

void
github_create_release(const gcli_new_release *release)
{
	char               *url            = NULL;
	char               *e_owner        = NULL;
	char               *e_repo        = NULL;
	char               *upload_url     = NULL;
	char               *post_data      = NULL;
	char               *name_json      = NULL;
	char               *commitish_json = NULL;
	sn_sv               escaped_body   = {0};
	gcli_fetch_buffer  buffer         = {0};
	gcli_release       response       = {0};

	assert(release);

	e_owner = gcli_urlencode(release->owner);
	e_repo  = gcli_urlencode(release->repo);

	/* https://docs.github.com/en/rest/reference/repos#create-a-release */
	url = sn_asprintf(
		"%s/repos/%s/%s/releases",
		gcli_get_apibase(), e_owner, e_repo);

	escaped_body = gcli_json_escape(release->body);

	if (release->commitish)
		commitish_json = sn_asprintf(
			",\"target_commitish\": \"%s\"",
			release->commitish);

	if (release->name)
		name_json = sn_asprintf(
			",\"name\": \"%s\"",
			release->name);

	post_data = sn_asprintf(
		"{"
		"    \"tag_name\": \"%s\","
		"    \"draft\": %s,"
		"    \"prerelease\": %s,"
		"    \"body\": \""SV_FMT"\""
		"    %s"
		"    %s"
		"}",
		release->tag,
		gcli_json_bool(release->draft),
		gcli_json_bool(release->prerelease),
		SV_ARGS(escaped_body),
		commitish_json ? commitish_json : "",
		name_json ? name_json : "");

	gcli_fetch_with_method("POST", url, post_data, NULL, &buffer);
	github_parse_single_release(buffer, &response);

	printf("INFO : Release at "SV_FMT"\n", SV_ARGS(response.html_url));

	upload_url = github_get_upload_url(&response);

	for (size_t i = 0; i < release->assets_size; ++i) {
		printf("INFO : Uploading asset %s...\n", release->assets[i].path);
		github_upload_release_asset(upload_url, release->assets[i]);
	}

	free(upload_url);
	free(buffer.data);
	free(url);
	free(post_data);
	free(escaped_body.data);
	free(e_owner);
	free(e_repo);
	free(name_json);
	free(commitish_json);
}

void
github_delete_release(const char *owner, const char *repo, const char *id)
{
	char               *url     = NULL;
	char               *e_owner = NULL;
	char               *e_repo  = NULL;
	gcli_fetch_buffer  buffer  = {0};

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/releases/%s",
		gcli_get_apibase(), e_owner, e_repo, id);

	gcli_fetch_with_method("DELETE", url, NULL, NULL, &buffer);

	free(url);
	free(e_owner);
	free(e_repo);
	free(buffer.data);
}
