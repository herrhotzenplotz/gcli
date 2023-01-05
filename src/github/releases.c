/*
 * Copyright 2021, 2022 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <templates/github/releases.h>

int
github_get_releases(char const *owner,
                    char const *repo,
                    int const max,
                    gcli_release_list *const list)
{
	char               *url      = NULL;
	char               *e_owner  = NULL;
	char               *e_repo   = NULL;
	char               *next_url = NULL;
	gcli_fetch_buffer   buffer   = {0};
	struct json_stream  stream   = {0};


	*list = (gcli_release_list) {0};

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/releases",
		gcli_get_apibase(),
		e_owner, e_repo);

	do {
		gcli_fetch(url, &next_url, &buffer);
		json_open_buffer(&stream, buffer.data, buffer.length);
		parse_github_releases(&stream, &list->releases, &list->releases_size);

		json_close(&stream);
		free(url);
		free(buffer.data);
	} while ((url = next_url) && (max == -1 || (int)list->releases_size < max));

	free(next_url);
	free(e_owner);
	free(e_repo);

	return 0;
}

static void
github_parse_single_release(gcli_fetch_buffer buffer, gcli_release *const out)
{
	struct json_stream stream = {0};

	json_open_buffer(&stream, buffer.data, buffer.length);
	json_set_streaming(&stream, 1);
	parse_github_release(&stream, out);
	json_close(&stream);
}

static char *
github_get_upload_url(gcli_release *const it)
{
	char *delim = strchr(it->upload_url.data, '{');
	if (delim == NULL)
		errx(1, "GitHub API returned an invalid upload url");

	size_t len = delim - it->upload_url.data;
	return sn_strndup(it->upload_url.data, len);
}

static void
github_upload_release_asset(char const *url,
                            gcli_release_asset_upload const asset)
{
	char              *req          = NULL;
	sn_sv              file_content = {0};
	gcli_fetch_buffer  buffer       = {0};

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
github_create_release(gcli_new_release const *release)
{
	char              *url            = NULL;
	char              *e_owner        = NULL;
	char              *e_repo         = NULL;
	char              *upload_url     = NULL;
	char              *post_data      = NULL;
	char              *name_json      = NULL;
	char              *commitish_json = NULL;
	sn_sv              escaped_body   = {0};
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
github_delete_release(char const *owner, char const *repo, char const *id)
{
	char              *url     = NULL;
	char              *e_owner = NULL;
	char              *e_repo  = NULL;
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
