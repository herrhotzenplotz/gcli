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

#include <gcli/curl.h>
#include <gcli/github/config.h>
#include <gcli/github/releases.h>
#include <gcli/json_util.h>
#include <pdjson/pdjson.h>

#include <assert.h>

#include <templates/github/releases.h>

int
github_get_releases(gcli_ctx *ctx, char const *owner, char const *repo,
                    int const max, gcli_release_list *const list)
{
	char *url = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;

	gcli_fetch_list_ctx fl = {
		.listp = &list->releases,
		.sizep = &list->releases_size,
		.max = max,
		.parse = (parsefn)(parse_github_releases),
	};

	*list = (gcli_release_list) {0};

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/releases",
		gcli_get_apibase(ctx),
		e_owner, e_repo);

	free(e_owner);
	free(e_repo);

	return gcli_fetch_list(ctx, url, &fl);
}

static void
github_parse_single_release(gcli_ctx *ctx, gcli_fetch_buffer buffer,
                            gcli_release *const out)
{
	struct json_stream stream = {0};

	json_open_buffer(&stream, buffer.data, buffer.length);
	json_set_streaming(&stream, 1);
	parse_github_release(ctx, &stream, out);
	json_close(&stream);
}

static int
github_get_upload_url(gcli_ctx *ctx, gcli_release *const it, char **out)
{
	char *delim = strchr(it->upload_url.data, '{');
	if (delim == NULL)
		return gcli_error(ctx, "GitHub API returned an invalid upload url");

	size_t len = delim - it->upload_url.data;
	*out = sn_strndup(it->upload_url.data, len);

	return 0;
}

static int
github_upload_release_asset(gcli_ctx *ctx, char const *url,
                            gcli_release_asset_upload const asset)
{
	char *req = NULL;
	sn_sv file_content = {0};
	gcli_fetch_buffer buffer = {0};
	int rc = 0;

	file_content.length = sn_mmap_file(asset.path, (void **)&file_content.data);
	if (file_content.length == 0)
		return -1;

	/* TODO: URL escape this */
	req = sn_asprintf("%s?name=%s", url, asset.name);

	rc = gcli_post_upload(
		ctx,
		req,
		"application/octet-stream", /* HACK */
		file_content.data,
		file_content.length,
		&buffer);

	free(req);
	free(buffer.data);

	return rc;
}

int
github_create_release(gcli_ctx *ctx, gcli_new_release const *release)
{
	char *url = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	char *upload_url = NULL;
	char *post_data = NULL;
	char *name_json = NULL;
	char *commitish_json = NULL;
	sn_sv escaped_body = {0};
	gcli_fetch_buffer buffer = {0};
	gcli_release response = {0};
	int rc = 0;

	assert(release);

	e_owner = gcli_urlencode(release->owner);
	e_repo = gcli_urlencode(release->repo);

	/* https://docs.github.com/en/rest/reference/repos#create-a-release */
	url = sn_asprintf(
		"%s/repos/%s/%s/releases",
		gcli_get_apibase(ctx), e_owner, e_repo);

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

	rc = gcli_fetch_with_method(ctx, "POST", url, post_data, NULL, &buffer);
	if (rc < 0)
		goto out;

	github_parse_single_release(ctx, buffer, &response);

    rc = github_get_upload_url(ctx, &response, &upload_url);
	if (rc < 0)
		goto out;

	for (size_t i = 0; i < release->assets_size; ++i) {
		printf("INFO : Uploading asset %s...\n", release->assets[i].path);
		rc = github_upload_release_asset(ctx, upload_url, release->assets[i]);

		if (rc < 0)
			break;
	}

out:
	free(upload_url);
	free(buffer.data);
	free(url);
	free(post_data);
	free(escaped_body.data);
	free(e_owner);
	free(e_repo);
	free(name_json);
	free(commitish_json);

	return rc;
}

int
github_delete_release(gcli_ctx *ctx, char const *owner, char const *repo,
                      char const *id)
{
	char *url = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	int rc = 0;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/releases/%s",
		gcli_get_apibase(ctx), e_owner, e_repo, id);

	rc = gcli_fetch_with_method(ctx, "DELETE", url, NULL, NULL, NULL);

	free(url);
	free(e_owner);
	free(e_repo);

	return rc;
}
