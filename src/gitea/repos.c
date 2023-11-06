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

#include <gcli/gitea/repos.h>
#include <gcli/github/repos.h>

#include <gcli/curl.h>

#include <assert.h>

int
gitea_get_repos(gcli_ctx *ctx, char const *owner, int const max,
                gcli_repo_list *const list)
{
	return github_get_repos(ctx, owner, max, list);
}

int
gitea_get_own_repos(gcli_ctx *ctx, int const max, gcli_repo_list *const list)
{
	return github_get_own_repos(ctx, max, list);
}

int
gitea_repo_create(gcli_ctx *ctx, gcli_repo_create_options const *options,
                  gcli_repo *const out)
{
	return github_repo_create(ctx, options, out);
}

int
gitea_repo_delete(gcli_ctx *ctx, char const *owner, char const *repo)
{
	return github_repo_delete(ctx, owner, repo);
}

/* Unlike Github and Gitlab, Gitea only supports private or non-private
 * (thus public) repositories. Separate implementation required. */
int
gitea_repo_set_visibility(gcli_ctx *ctx, char const *const owner,
                          char const *const repo, gcli_repo_visibility vis)
{
	char *url;
	char *e_owner, *e_repo;
	bool is_private;
	char *payload;
	int rc;

	switch (vis) {
	case GCLI_REPO_VISIBILITY_PRIVATE:
		is_private = true;
		break;
	case GCLI_REPO_VISIBILITY_PUBLIC:
		is_private = false;
		break;
	default:
		assert(false && "Invalid visibility");
		return gcli_error(ctx, "bad or unsupported visibility level for Gitea");
	}

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/repos/%s/%s", gcli_get_apibase(ctx), e_owner, e_repo);
	payload = sn_asprintf("{ \"private\": %s }", is_private ? "true" : "false");

	rc = gcli_fetch_with_method(ctx, "PATCH", url, payload, NULL, NULL);

	free(payload);
	free(e_owner);
	free(e_repo);
	free(url);

	return rc;
}
