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

#include <gcli/gitea/pulls.h>
#include <gcli/github/pulls.h>
#include <gcli/github/issues.h>

int
gitea_get_pulls(gcli_ctx *ctx, char const *owner, char const *repo,
                gcli_pull_fetch_details const *const details, int const max,
                gcli_pull_list *const out)
{
	return github_get_pulls(ctx, owner, repo, details, max, out);
}

int
gitea_get_pull(gcli_ctx *ctx, char const *owner, char const *repo,
               gcli_id const pr_number, gcli_pull *const out)
{
	return github_get_pull(ctx, owner, repo, pr_number, out);
}

int
gitea_get_pull_commits(gcli_ctx *ctx, char const *owner, char const *repo,
                       gcli_id const pr_number, gcli_commit_list *const out)
{
	return github_get_pull_commits(ctx, owner, repo, pr_number, out);
}

int
gitea_pull_submit(gcli_ctx *ctx, gcli_submit_pull_options opts)
{
	warnx("In case the following process errors out, see: "
	      "https://github.com/go-gitea/gitea/issues/20175");
	return github_perform_submit_pull(ctx, opts);
}

int
gitea_pull_merge(gcli_ctx *ctx, char const *owner, char const *repo,
                 gcli_id const pr_number, enum gcli_merge_flags const flags)
{
	int rc = 0;
	char *url = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	char *data = NULL;
	bool const squash = flags & GCLI_PULL_MERGE_SQUASH;
	bool const delete_branch = flags & GCLI_PULL_MERGE_DELETEHEAD;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);
	url = sn_asprintf("%s/repos/%s/%s/pulls/%lu/merge",
	                  gcli_get_apibase(ctx), e_owner, e_repo, pr_number);
	data = sn_asprintf("{ \"Do\": \"%s\", \"delete_branch_after_merge\": %s }",
	                   squash ? "squash" : "merge",
	                   delete_branch ? "true" : "false");

	rc = gcli_fetch_with_method(ctx, "POST", url, data, NULL, NULL);

	free(url);
	free(e_owner);
	free(e_repo);
	free(data);

	return rc;
}

static int
gitea_pulls_patch_state(gcli_ctx *ctx, char const *owner, char const *repo,
                        int const pr_number, char const *state)
{
	char *url = NULL;
	char *data = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	int rc = 0;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/pulls/%d",
		gcli_get_apibase(ctx),
		e_owner, e_repo,
		pr_number);
	data = sn_asprintf("{ \"state\": \"%s\"}", state);

	rc = gcli_fetch_with_method(ctx, "PATCH", url, data, NULL, NULL);

	free(data);
	free(url);
	free(e_owner);
	free(e_repo);

	return rc;
}

int
gitea_pull_close(gcli_ctx *ctx, char const *owner, char const *repo,
                 gcli_id const pr_number)
{
	return gitea_pulls_patch_state(ctx, owner, repo, pr_number, "closed");
}

int
gitea_pull_reopen(gcli_ctx *ctx, char const *owner, char const *repo,
                  gcli_id const pr_number)
{
	return gitea_pulls_patch_state(ctx, owner, repo, pr_number, "open");
}

int
gitea_print_pr_diff(gcli_ctx *ctx, FILE *const stream, char const *owner,
                    char const *repo, gcli_id const pr_number)
{
	char *url = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	int rc = 0;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/pulls/%lu.patch",
		gcli_get_apibase(ctx),
		e_owner, e_repo, pr_number);

	rc = gcli_curl(ctx, stream, url, NULL);

	free(e_owner);
	free(e_repo);
	free(url);

	return rc;
}

int
gitea_pull_get_checks(gcli_ctx *ctx, char const *owner, char const *repo,
                      gcli_id const pr_number, gcli_pull_checks_list *out)
{
	(void) ctx;
	(void) owner;
	(void) repo;
	(void) pr_number;
	(void) out;

	return gcli_error(ctx, "Pull Request checks are not available on Gitea");
}

int
gitea_pull_set_milestone(gcli_ctx *ctx, char const *owner, char const *repo,
                         gcli_id pr_number, gcli_id milestone_id)
{
	return github_issue_set_milestone(ctx, owner, repo, pr_number,
	                                  milestone_id);
}

int
gitea_pull_clear_milestone(gcli_ctx *ctx, char const *owner, char const *repo,
                           gcli_id pr_number)
{
	/* NOTE: The github routine for clearing issues sets the milestone
	 * to null (not the integer zero). However this does not work in
	 * the case of Gitea which clear the milestone by setting it to
	 * the integer value zero. */
	return github_issue_set_milestone(ctx, owner, repo, pr_number, 0);
}

int
gitea_pull_add_reviewer(gcli_ctx *ctx, char const *owner, char const *repo,
                        gcli_id pr_number, char const *username)
{
	return github_pull_add_reviewer(ctx, owner, repo, pr_number, username);
}
