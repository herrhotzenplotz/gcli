/*
 * Copyright 2021-2025 Nico Sonack <nsonack@herrhotzenplotz.de>
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
#include <gcli/github/repos.h>
#include <gcli/json_gen.h>
#include <gcli/json_util.h>
#include <gcli/port/util.h>
#include <pdjson.h>

#include <assert.h>

#include <templates/github/releases.h>

int
github_get_releases(struct gcli_ctx *ctx, struct gcli_path const *const path,
                    int const max, struct gcli_release_list *const list)
{
	char *url = NULL;
	int rc = 0;

	struct gcli_fetch_list_ctx fl = {
		.listp = &list->releases,
		.sizep = &list->releases_size,
		.max = max,
		.parse = (parsefn)(parse_github_releases),
	};

	*list = (struct gcli_release_list) {0};

	rc = github_repo_make_url(ctx, path, &url, "/releases");
	if (rc < 0)
		return rc;

	return gcli_fetch_list(ctx, url, &fl);
}

static void
github_parse_single_release(struct gcli_ctx *ctx, struct gcli_fetch_buffer buffer,
                            struct gcli_release *const out)
{
	struct json_stream stream = {0};

	json_open_buffer(&stream, buffer.data, buffer.length);
	json_set_streaming(&stream, 1);
	parse_github_release(ctx, &stream, out);
	json_close(&stream);
}

static int
github_get_upload_url(struct gcli_ctx *ctx, struct gcli_release *const it,
                      char **out)
{
	char *delim = strchr(it->upload_url, '{');
	if (delim == NULL)
		return gcli_error(ctx, "GitHub API returned an invalid upload url");

	size_t len = delim - it->upload_url;
	*out = gcli_strndup(it->upload_url, len);

	return 0;
}

static int
github_upload_release_asset(struct gcli_ctx *ctx, char const *url,
                            struct gcli_release_asset_upload const asset)
{
	char *req = NULL;
	gcli_sv file_content = {0};
	struct gcli_fetch_buffer buffer = {0};
	int rc = 0;

	file_content.length = gcli_read_file(asset.path, &file_content.data);
	if (file_content.length == 0)
		return -1;

	/* TODO: URL escape this */
	req = gcli_asprintf("%s?name=%s", url, asset.name);

	rc = gcli_post_upload(
		ctx,
		req,
		"application/octet-stream", /* HACK */
		file_content.data,
		file_content.length,
		&buffer);

	gcli_clear_ptr(&req);
	gcli_fetch_buffer_free(&buffer);

	return rc;
}

int
github_create_release(struct gcli_ctx *ctx,
                      struct gcli_create_release_args const *release)
{
	char *url = NULL, *upload_url = NULL, *payload = NULL;
	int rc = 0;
	struct gcli_fetch_buffer buffer = {0};
	struct gcli_jsongen gen = {0};
	struct gcli_release response = {0};

	/* Payload */
	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "tag_name");
		gcli_jsongen_string(&gen, release->tag);

		gcli_jsongen_objmember(&gen, "draft");
		gcli_jsongen_bool(&gen, release->draft);

		gcli_jsongen_objmember(&gen, "prerelease");
		gcli_jsongen_bool(&gen, release->prerelease);

		if (release->body) {
			gcli_jsongen_objmember(&gen, "body");
			gcli_jsongen_string(&gen, release->body);
		}

		if (release->commitish) {
			gcli_jsongen_objmember(&gen, "target_commitish");
			gcli_jsongen_string(&gen, release->commitish);
		}

		if (release->name) {
			gcli_jsongen_objmember(&gen, "name");
			gcli_jsongen_string(&gen, release->name);
		}
	}
	gcli_jsongen_end_object(&gen);
	payload = gcli_jsongen_to_string(&gen);
	gcli_jsongen_free(&gen);

	rc = github_repo_make_url(ctx, &release->repo_path, &url, "/releases");
	if (rc < 0)
		goto out;

	rc = gcli_fetch_with_method(ctx, "POST", url, payload, NULL, &buffer);
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
	gcli_release_free(&response);
	gcli_fetch_buffer_free(&buffer);
	gcli_clear_ptr(&upload_url);
	gcli_clear_ptr(&url);
	gcli_clear_ptr(&payload);

	return rc;
}

int
github_delete_release(struct gcli_ctx *ctx, struct gcli_path const *const path,
                      char const *const id)
{
	char *url = NULL;
	int rc = 0;

	rc = github_repo_make_url(ctx, path, &url, "/releases/%s", id);
	if (rc < 0)
		return rc;

	rc = gcli_fetch_with_method(ctx, "DELETE", url, NULL, NULL, NULL);

	gcli_clear_ptr(&url);

	return rc;
}
