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

#include <gcli/forges.h>
#include <gcli/github/issues.h>
#include <gcli/issues.h>
#include <gcli/json_util.h>
#include <sn/sn.h>

void
gcli_issue_free(gcli_issue *const it)
{
	free(it->title.data);
	free(it->created_at.data);
	free(it->author.data);
	free(it->state.data);
	free(it->body.data);
	free(it->milestone.data);

	for (size_t i = 0; i < it->labels_size; ++i)
		free(it->labels[i].data);

	free(it->labels);
}

void
gcli_issues_free(gcli_issue_list *const list)
{
	for (size_t i = 0; i < list->issues_size; ++i)
		gcli_issue_free(&list->issues[i]);

	free(list->issues);

	list->issues = NULL;
	list->issues_size = 0;
}

int
gcli_get_issues(gcli_ctx *ctx, char const *owner, char const *repo,
                gcli_issue_fetch_details const *details, int const max,
                gcli_issue_list *const out)
{
	return gcli_forge(ctx)->get_issues(ctx, owner, repo, details, max, out);
}

int
gcli_get_issue(gcli_ctx *ctx, char const *owner, char const *repo,
               gcli_id const issue_number, gcli_issue *const out)
{
	return gcli_forge(ctx)->get_issue_summary(
		ctx, owner, repo, issue_number, out);
}

int
gcli_issue_close(gcli_ctx *ctx, char const *owner, char const *repo,
                 gcli_id const issue_number)
{
	return gcli_forge(ctx)->issue_close(ctx, owner, repo, issue_number);
}

int
gcli_issue_reopen(gcli_ctx *ctx, char const *owner, char const *repo,
                  gcli_id const issue_number)
{
	return gcli_forge(ctx)->issue_reopen(ctx, owner, repo, issue_number);
}

int
gcli_issue_submit(gcli_ctx *ctx, gcli_submit_issue_options opts)
{
	gcli_fetch_buffer json_buffer = {0};
	int rc = 0;

	rc = gcli_forge(ctx)->perform_submit_issue(ctx, opts, &json_buffer);
	free(json_buffer.data);

	return rc;
}

int
gcli_issue_assign(gcli_ctx *ctx, char const *owner, char const *repo,
                  gcli_id const issue_number, char const *assignee)
{
	return gcli_forge(ctx)->issue_assign(ctx, owner, repo, issue_number, assignee);
}

int
gcli_issue_add_labels(gcli_ctx *ctx, char const *owner, char const *repo,
                      gcli_id const issue, char const *const labels[],
                      size_t const labels_size)
{
	return gcli_forge(ctx)->issue_add_labels(ctx,owner, repo, issue, labels,
	                                         labels_size);
}

int
gcli_issue_remove_labels(gcli_ctx *ctx, char const *owner, char const *repo,
                         gcli_id const issue, char const *const labels[],
                         size_t const labels_size)
{
	return gcli_forge(ctx)->issue_remove_labels(
		ctx, owner, repo, issue, labels, labels_size);
}

int
gcli_issue_set_milestone(gcli_ctx *ctx, char const *const owner,
                         char const *const repo, gcli_id const issue,
                         int const milestone)
{
	return gcli_forge(ctx)->issue_set_milestone(
		ctx, owner, repo, issue, milestone);
}

int
gcli_issue_clear_milestone(gcli_ctx *ctx, char const *const owner,
                           char const *const repo, gcli_id const issue)
{
	return gcli_forge(ctx)->issue_clear_milestone(ctx, owner, repo, issue);
}
