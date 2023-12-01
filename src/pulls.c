/*
 * Copyright 2021,2022 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/forges.h>
#include <gcli/github/checks.h>
#include <gcli/github/pulls.h>
#include <gcli/gitlab/pipelines.h>
#include <gcli/json_util.h>
#include <gcli/pulls.h>
#include <sn/sn.h>

#include <assert.h>

void
gcli_pulls_free(gcli_pull_list *const it)
{
	for (size_t i = 0; i < it->pulls_size; ++i)
		gcli_pull_free(&it->pulls[i]);

	free(it->pulls);

	it->pulls = NULL;
	it->pulls_size = 0;
}

int
gcli_get_pulls(gcli_ctx *ctx, char const *owner, char const *repo,
               gcli_pull_fetch_details const *const details, int const max,
               gcli_pull_list *const out)
{
	return gcli_forge(ctx)->get_pulls(ctx, owner, repo, details, max, out);
}

int
gcli_pull_get_diff(gcli_ctx *ctx, FILE *stream, char const *owner,
                   char const *reponame, gcli_id const pr_number)
{
	return gcli_forge(ctx)->print_pull_diff(ctx, stream, owner, reponame, pr_number);
}

int
gcli_pull_get_commits(gcli_ctx *ctx, char const *owner, char const *repo,
                      gcli_id const pr_number, gcli_commit_list *const out)
{
	return gcli_forge(ctx)->get_pull_commits(ctx, owner, repo, pr_number, out);
}

void
gcli_commits_free(gcli_commit_list *list)
{
	for (size_t i = 0; i < list->commits_size; ++i) {
		free(list->commits[i].sha);
		free(list->commits[i].long_sha);
		free(list->commits[i].message);
		free(list->commits[i].date);
		free(list->commits[i].author);
		free(list->commits[i].email);
	}

	free(list->commits);

	list->commits = NULL;
	list->commits_size = 0;
}

void
gcli_pull_free(gcli_pull *const it)
{
	free(it->author);
	free(it->state);
	free(it->title);
	free(it->body);
	free(it->created_at);
	free(it->commits_link);
	free(it->head_label);
	free(it->base_label);
	free(it->head_sha);
	free(it->base_sha);
	free(it->milestone);
	free(it->coverage);

	for (size_t i = 0; i < it->labels_size; ++i)
		free(it->labels[i].data);

	free(it->labels);
}

int
gcli_get_pull(gcli_ctx *ctx, char const *owner, char const *repo,
              gcli_id const pr_number, gcli_pull *const out)
{
	return gcli_forge(ctx)->get_pull(ctx, owner, repo, pr_number, out);
}

int
gcli_pull_get_checks(gcli_ctx *ctx, char const *owner, char const *repo,
                     gcli_id const pr_number, gcli_pull_checks_list *out)
{
	return gcli_forge(ctx)->get_pull_checks(ctx, owner, repo, pr_number, out);
}

void
gcli_pull_checks_free(gcli_pull_checks_list *list)
{
	switch (list->forge_type) {
	case GCLI_FORGE_GITHUB:
		github_free_checks((github_check_list *)list);
		break;
	case GCLI_FORGE_GITLAB:
		gitlab_pipelines_free((gitlab_pipeline_list *)list);
		break;
	default:
		assert(0 && "unreachable");
	}
}

int
gcli_pull_submit(gcli_ctx *ctx, gcli_submit_pull_options opts)
{
	return gcli_forge(ctx)->perform_submit_pull(ctx, opts);
}

int
gcli_pull_merge(gcli_ctx *ctx, char const *owner, char const *reponame,
                gcli_id const pr_number, enum gcli_merge_flags flags)
{
	return gcli_forge(ctx)->pull_merge(ctx, owner, reponame, pr_number, flags);
}

int
gcli_pull_close(gcli_ctx *ctx, char const *owner, char const *reponame,
                gcli_id const pr_number)
{
	return gcli_forge(ctx)->pull_close(ctx, owner, reponame, pr_number);
}

int
gcli_pull_reopen(gcli_ctx *ctx, char const *owner, char const *reponame,
                 gcli_id const pr_number)
{
	return gcli_forge(ctx)->pull_reopen(ctx, owner, reponame, pr_number);
}

int
gcli_pull_add_labels(gcli_ctx *ctx, char const *owner, char const *repo,
                     gcli_id const pr_number, char const *const labels[],
                     size_t const labels_size)
{
	return gcli_forge(ctx)->pull_add_labels(
		ctx, owner, repo, pr_number, labels, labels_size);
}

int
gcli_pull_remove_labels(gcli_ctx *ctx, char const *owner, char const *repo,
                        gcli_id const pr_number, char const *const labels[],
                        size_t const labels_size)
{
	return gcli_forge(ctx)->pull_remove_labels(
		ctx, owner, repo, pr_number, labels, labels_size);
}

int
gcli_pull_set_milestone(gcli_ctx *ctx, char const *owner, char const *repo,
                        gcli_id const pr_number, int milestone_id)
{
	return gcli_forge(ctx)->pull_set_milestone(
		ctx, owner, repo, pr_number, milestone_id);
}

int
gcli_pull_clear_milestone(gcli_ctx *ctx, char const *owner, char const *repo,
                          gcli_id const pr_number)
{
	return gcli_forge(ctx)->pull_clear_milestone(ctx, owner, repo, pr_number);
}

int
gcli_pull_add_reviewer(gcli_ctx *ctx, char const *owner, char const *repo,
                       gcli_id pr_number, char const *username)
{
	return gcli_forge(ctx)->pull_add_reviewer(
		ctx, owner, repo, pr_number, username);
}

int
gcli_pull_get_patch(gcli_ctx *ctx, FILE *out, char const *owner, char const *repo,
                    gcli_id pull_id)
{
	return gcli_forge(ctx)->pull_get_patch(ctx, out, owner, repo, pull_id);
}

int
gcli_pull_set_title(gcli_ctx *ctx, char const *const owner,
                    char const *const repo, gcli_id const pull,
                    char const *new_title)
{
	return gcli_forge(ctx)->pull_set_title(ctx, owner, repo, pull, new_title);
}
