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

#include <gcli/gitlab/config.h>
#include <gcli/gitlab/repos.h>
#include <gcli/json_util.h>

#include <pdjson/pdjson.h>
#include <sn/sn.h>

#include <templates/gitlab/repos.h>

int
gitlab_get_repo(gcli_ctx *ctx, char const *owner, char const *repo,
                gcli_repo *const out)
{
	/* GET /projects/:id */
	char *url = NULL;
	gcli_fetch_buffer buffer = {0};
	json_stream stream = {0};
	char *e_owner = {0};
	char *e_repo = {0};
	int rc;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/projects/%s%%2F%s",
		gitlab_get_apibase(ctx),
	    e_owner, e_repo);

	rc = gcli_fetch(ctx, url, NULL, &buffer);

	if (rc == 0) {
		json_open_buffer(&stream, buffer.data, buffer.length);
		parse_gitlab_repo(ctx, &stream, out);
		json_close(&stream);
	}

	free(buffer.data);
	free(e_owner);
	free(e_repo);
	free(url);

	return rc;
}

static void
gitlab_repos_fixup_missing_visibility(gcli_repo_list *const list)
{
	static char const public[] = "public";
	static size_t const public_len = sizeof(public) - 1;

	/* Gitlab does not return a visibility field in the repo object on
	 * unauthenticated API requests. We fix up the missing field here
	 * assuming that the repository must be public. */
	for (size_t i = 0; i < list->repos_size; ++i) {
		if (sn_sv_null(list->repos[i].visibility))
			list->repos[i].visibility = (sn_sv) {
				.data = strdup(public),
				.length = public_len,
			};
	}
}

int
gitlab_get_repos(gcli_ctx *ctx, char const *owner, int const max,
                 gcli_repo_list *const list)
{
	char *url = NULL;
	char *e_owner = NULL;
	int rc = 0;
	gcli_fetch_list_ctx fl = {
		.listp = &list->repos,
		.sizep = &list->repos_size,
		.parse = (parsefn)(parse_gitlab_repos),
		.max = max,
	};

	e_owner = gcli_urlencode(owner);
	url = sn_asprintf("%s/users/%s/projects", gitlab_get_apibase(ctx), e_owner);
	free(e_owner);

	rc = gcli_fetch_list(ctx, url, &fl);

	if (rc == 0)
		gitlab_repos_fixup_missing_visibility(list);

	return rc;
}

int
gitlab_get_own_repos(gcli_ctx *ctx, int const max, gcli_repo_list *const out)
{
	char *_account = NULL;
	sn_sv account = {0};
	int n;

	account = gitlab_get_account(ctx);
	if (!account.length)
		errx(1, "error: gitlab.account is not set");

	_account = sn_sv_to_cstr(account);

	n = gitlab_get_repos(ctx, _account, max, out);

	free(_account);

	return n;
}

int
gitlab_repo_delete(gcli_ctx *ctx, char const *owner, char const *repo)
{
	char *url = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	int rc = 0;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s",
	                  gitlab_get_apibase(ctx),
	                  e_owner, e_repo);

	rc = gcli_fetch_with_method(ctx, "DELETE", url, NULL, NULL, NULL);

	free(url);
	free(e_owner);
	free(e_repo);

	return rc;
}

int
gitlab_repo_create(gcli_ctx *ctx, gcli_repo_create_options const *options,
                   gcli_repo *out)
{
	char *url, *data;
	gcli_fetch_buffer buffer = {0};
	json_stream stream = {0};
	int rc;

	/* Request preparation */
	url = sn_asprintf("%s/projects", gitlab_get_apibase(ctx));
	/* TODO: escape the repo name and the description */
	data = sn_asprintf("{\"name\": \""SV_FMT"\","
	                   " \"description\": \""SV_FMT"\","
	                   " \"visibility\": \"%s\" }",
	                   SV_ARGS(options->name),
	                   SV_ARGS(options->description),
	                   options->private ? "private" : "public");

	/* Fetch and parse result */
	rc = gcli_fetch_with_method(ctx, "POST", url, data, NULL, out ? &buffer : NULL);
	if (rc == 0 && out) {
		json_open_buffer(&stream, buffer.data, buffer.length);
		parse_gitlab_repo(ctx, &stream, out);

		json_close(&stream);
	}

	free(buffer.data);
	free(data);
	free(url);

	return rc;
}
