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
#include <gcli/gitlab/config.h>
#include <gcli/gitlab/releases.h>
#include <gcli/json_util.h>

#include <templates/gitlab/releases.h>

#include <pdjson/pdjson.h>

#include <string.h>

static void
fixup_asset_name(gcli_ctx *ctx, gcli_release_asset *const asset)
{
	if (!asset->name) {
		asset->name = gcli_urldecode(ctx, strrchr(asset->url, '/') + 1);
	}
}

static void
fixup_release_asset_names(gcli_ctx *ctx, gcli_release_list *list)
{
	/* Iterate over releases */
	for (size_t j = 0; j < list->releases_size; ++j) {
		/* iterate over releases */
		for (size_t i = 0; i < list->releases[j].assets_size; ++i) {
			fixup_asset_name(ctx, &list->releases[j].assets[i]);
		}
	}
}

int
gitlab_get_releases(gcli_ctx *ctx, char const *owner, char const *repo,
                    int const max, gcli_release_list *const list)
{
	char *url = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	int rc = 0;

	gcli_fetch_list_ctx fl = {
		.listp = &list->releases,
		.sizep = &list->releases_size,
		.max = max,
		.parse = (parsefn)(parse_gitlab_releases),
	};

	*list = (gcli_release_list) {0};

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/releases", gcli_get_apibase(ctx),
	                  e_owner, e_repo);

	free(e_owner);
	free(e_repo);

	rc = gcli_fetch_list(ctx, url, &fl);

	if (rc == 0)
		fixup_release_asset_names(ctx, list);

	return rc;
}

int
gitlab_create_release(gcli_ctx *ctx, gcli_new_release const *release)
{
	char *url = NULL;
	char *upload_url = NULL;
	char *post_data = NULL;
	char *name_json = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	char *commitish_json = NULL;
	sn_sv escaped_body = {0};
	int rc = 0;

	e_owner = gcli_urlencode(release->owner);
	e_repo  = gcli_urlencode(release->repo);

	/* https://docs.github.com/en/rest/reference/repos#create-a-release */
	url = sn_asprintf("%s/projects/%s%%2F%s/releases", gcli_get_apibase(ctx),
	                  e_owner, e_repo);

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

	rc = gcli_fetch_with_method(ctx, "POST", url, post_data, NULL, NULL);

	if (release->assets_size)
		warnx("GitLab release asset uploads are not yet supported");

	free(upload_url);
	free(url);
	free(post_data);
	free(escaped_body.data);
	free(name_json);
	free(commitish_json);
	free(e_owner);
	free(e_repo);

	return rc;
}

int
gitlab_delete_release(gcli_ctx *ctx, char const *owner, char const *repo, char const *id)
{
	char *url     = NULL;
	char *e_owner = NULL;
	char *e_repo  = NULL;
	int   rc      = 0;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/releases/%s", gcli_get_apibase(ctx),
	                  e_owner, e_repo, id);

	rc = gcli_fetch_with_method(ctx, "DELETE", url, NULL, NULL, NULL);

	free(url);
	free(e_owner);
	free(e_repo);

	return rc;
}
