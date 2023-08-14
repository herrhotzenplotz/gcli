/*
 * Copyright 2022 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/gitea/releases.h>
#include <gcli/github/releases.h>

#include <gcli/config.h>
#include <gcli/curl.h>
#include <gcli/json_util.h>

#include <templates/github/releases.h>

int
gitea_get_releases(char const *owner, char const *repo,
                   int const max, gcli_release_list *const list)
{
	return github_get_releases(owner, repo, max, list);
}

static void
gitea_parse_release(gcli_fetch_buffer const *const buffer,
                    gcli_release *const out)
{
	json_stream stream = {0};
	json_open_buffer(&stream, buffer->data, buffer->length);
	parse_github_release(&stream, out);
	json_close(&stream);
}

static int
gitea_upload_release_asset(char *const url,
                           gcli_release_asset_upload const asset)
{
	char *e_assetname = NULL;
	char *request = NULL;
	gcli_fetch_buffer buffer = {0};
	int rc = 0;

	e_assetname = gcli_urlencode(asset.name);
	request = sn_asprintf("%s?name=%s", url, e_assetname);

	rc = gcli_curl_gitea_upload_attachment(request, asset.path, &buffer);

	free(request);
	free(e_assetname);
	free(buffer.data);

	return rc;
}

int
gitea_create_release(gcli_new_release const *release)
{
	char *commitish_json = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	char *name_json = NULL;
	char *post_data = NULL;
	char *upload_url = NULL;
	char *url = NULL;
	gcli_fetch_buffer buffer = {0};
	gcli_release response = {0};
	sn_sv escaped_body = {0};
	int rc = 0;

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
		name_json = sn_asprintf(",\"name\": \"%s\"", release->name);

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

	rc = gcli_fetch_with_method("POST", url, post_data, NULL, &buffer);
	if (rc < 0)
		goto out;

	gitea_parse_release(&buffer, &response);

	printf("INFO : Release at "SV_FMT"\n", SV_ARGS(response.html_url));

	upload_url = sn_asprintf("%s/repos/%s/%s/releases/"SV_FMT"/assets",
	                         gcli_get_apibase(), e_owner, e_repo,
	                         SV_ARGS(response.id));

	for (size_t i = 0; i < release->assets_size; ++i) {
		printf("INFO : Uploading asset %s...\n", release->assets[i].path);
		rc = gitea_upload_release_asset(upload_url, release->assets[i]);

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
gitea_delete_release(char const *owner, char const *repo, char const *id)
{
	return github_delete_release(owner, repo, id);
}
