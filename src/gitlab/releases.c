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

#include <gcli/curl.h>
#include <gcli/gitlab/config.h>
#include <gcli/gitlab/releases.h>
#include <gcli/json_util.h>

#include <pdjson/pdjson.h>

static void
gitlab_parse_assets_source(struct json_stream *input, gcli_release *out)
{
	enum json_type  next       = JSON_NULL;
	const char     *key;
	sn_sv           url        = {0};
	bool            is_tarball = false;

	if ((next = json_next(input)) != JSON_OBJECT)
		errx(1, "expected asset source object");

	while ((next = json_next(input)) == JSON_STRING) {
		size_t len;
		key = json_get_string(input, &len);

		if (strncmp(key, "format", len) == 0) {
			sn_sv format_name = get_sv(input);
			if (sn_sv_eq_to(format_name, "tar.bz2"))
				is_tarball = true;
			free(format_name.data);
		} else if (strncmp(key, "url", len) == 0) {
			url = get_sv(input);
		} else {
			SKIP_OBJECT_VALUE(input);
		}
	}

	if (next != JSON_OBJECT_END)
		errx(1, "unclosed asset source object");

	if (is_tarball) {
		out->tarball_url = url;
	} else {
		free(url.data);
	}
}

static void
gitlab_parse_asset_sources(struct json_stream *stream, gcli_release *out)
{
	enum json_type next = JSON_NULL;

	if ((next = json_next(stream)) != JSON_ARRAY)
		errx(1, "expected release assets sources array");

	while ((next = json_peek(stream)) == JSON_OBJECT) {
		gitlab_parse_assets_source(stream, out);
	}

	if (json_next(stream) != JSON_ARRAY_END)
		errx(1, "unclosed release assets sources array");
}

static void
gitlab_parse_assets(struct json_stream *input, gcli_release *out)
{
	enum json_type  next = JSON_NULL;
	const char     *key;

	if ((next = json_next(input)) != JSON_OBJECT)
		errx(1, "expected release assets object");

	while ((next = json_next(input)) == JSON_STRING) {
		size_t len;
		key = json_get_string(input, &len);

		if (strncmp(key, "sources", len) == 0)
			gitlab_parse_asset_sources(input, out);
		else
			SKIP_OBJECT_VALUE(input);
	}

	if (next != JSON_OBJECT_END)
		errx(1, "unclosed release assets object");
}

static void
gitlab_parse_release(struct json_stream *input, gcli_release *out)
{
	enum json_type  next = JSON_NULL;
	const char     *key;

	if ((next = json_next(input)) != JSON_OBJECT)
		errx(1, "expected release object");

	memset(out, 0, sizeof(*out));

	while ((next = json_next(input)) == JSON_STRING) {
		size_t len;
		key = json_get_string(input, &len);

		if (strncmp("name", key, len) == 0)
			out->name = get_sv(input);
		else if (strncmp("tag_name", key, len) == 0)
			out->id = get_sv(input);
		else if (strncmp("description", key, len) == 0)
			out->body = get_sv(input);
		else if (strncmp("assets", key, len) == 0)
			gitlab_parse_assets(input, out);
		else if (strncmp("author", key, len) == 0)
			out->author = get_user_sv(input);
		else if (strncmp("created_at", key, len) == 0)
			out->date = get_sv(input);
#if 0
		else if (strncmp("draft", key, len) == 0)
			;// Does not exist on gitlab
		else if (strncmp("prerelease", key, len) == 0)
			;// check the released_at field
		else if (strncmp("upload_url", key, len) == 0)
			;// doesn't exist on gitlab
		else if (strncmp("html_url", key, len) == 0)
			;// good luck
#endif
		else
			SKIP_OBJECT_VALUE(input);
	}

	if (next != JSON_OBJECT_END)
		errx(1, "unclosed release object");
}

int
gitlab_get_releases(
	const char     *owner,
	const char     *repo,
	int             max,
	gcli_release **out)
{
	char               *url      = NULL;
	char               *next_url = NULL;
	char               *e_owner  = NULL;
	char               *e_repo   = NULL;
	gcli_fetch_buffer  buffer   = {0};
	struct json_stream  stream   = {0};
	enum json_type      next     = JSON_NULL;
	int                 size     = 0;

	*out = NULL;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/projects/%s%%2F%s/releases",
		gitlab_get_apibase(),
		e_owner, e_repo);

	do {
		gcli_fetch(url, &next_url, &buffer);

		json_open_buffer(&stream, buffer.data, buffer.length);
		json_set_streaming(&stream, 1);

		if ((next = json_next(&stream)) != JSON_ARRAY)
			errx(1, "expected array of releases");

		while ((next = json_peek(&stream)) == JSON_OBJECT) {
			*out = realloc(*out, sizeof(**out) * (size + 1));
			gcli_release *it = &(*out)[size++];
			gitlab_parse_release(&stream, it);

			if (size == max)
				break;
		}

		json_close(&stream);
		free(url);
		free(buffer.data);
	} while ((url = next_url) && (max == -1 || size < max));

	free(e_owner);
	free(e_repo);
	free(next_url);

	return size;
}

void
gitlab_create_release(const gcli_new_release *release)
{
	char               *url            = NULL;
	char               *upload_url     = NULL;
	char               *post_data      = NULL;
	char               *name_json      = NULL;
	char               *e_owner        = NULL;
	char               *e_repo         = NULL;
	char               *commitish_json = NULL;
	sn_sv               escaped_body   = {0};
	gcli_fetch_buffer  buffer         = {0};

	e_owner = gcli_urlencode(release->owner);
	e_repo  = gcli_urlencode(release->repo);

	/* https://docs.github.com/en/rest/reference/repos#create-a-release */
	url = sn_asprintf(
		"%s/projects/%s%%2F%s/releases",
		gitlab_get_apibase(), e_owner, e_repo);

	escaped_body = gcli_json_escape(release->body);

	if (release->commitish)
		commitish_json = sn_asprintf(
			",\"ref\": \"%s\"",
			release->commitish);

	if (release->name)
		name_json = sn_asprintf(
			",\"name\": \"%s\"",
			release->name);

	/* Warnings because unsupported on gitlab */
	if (release->prerelease)
		warnx("prereleases are not supported on GitLab, option ignored");

	if (release->draft)
		warnx("draft releases are not supported on GitLab, option ignored");

	post_data = sn_asprintf(
		"{"
		"    \"tag_name\": \"%s\","
		"    \"description\": \""SV_FMT"\""
		"    %s"
		"    %s"
		"}",
		release->tag,
		SV_ARGS(escaped_body),
		commitish_json ? commitish_json : "",
		name_json ? name_json : "");

	gcli_fetch_with_method("POST", url, post_data, NULL, &buffer);

	if (release->assets_size)
		warnx("GitLab release asset uploads are not yet supported");

	free(upload_url);
	free(buffer.data);
	free(url);
	free(post_data);
	free(escaped_body.data);
	free(name_json);
	free(commitish_json);
	free(e_owner);
	free(e_repo);
}

void
gitlab_delete_release(const char *owner, const char *repo, const char *id)
{
	char               *url     = NULL;
	char               *e_owner = NULL;
	char               *e_repo  = NULL;
	gcli_fetch_buffer  buffer  = {0};

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/projects/%s%%2F%s/releases/%s",
		gitlab_get_apibase(),
		e_owner, e_repo, id);

	gcli_fetch_with_method("DELETE", url, NULL, NULL, &buffer);

	free(url);
	free(e_owner);
	free(e_repo);
	free(buffer.data);
}
