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

#include <gcli/forges.h>
#include <gcli/github/issues.h>
#include <gcli/issues.h>
#include <gcli/json_util.h>

void
gcli_issue_free(struct gcli_issue *const it)
{
	gcli_clear_ptr(&it->product);
	gcli_clear_ptr(&it->component);
	gcli_clear_ptr(&it->author);
	gcli_clear_ptr(&it->state);
	gcli_clear_ptr(&it->body);
	gcli_clear_ptr(&it->url);
	gcli_clear_ptr(&it->title);
	gcli_clear_ptr(&it->web_url);

	for (size_t i = 0; i < it->labels_size; ++i)
		gcli_clear_ptr(&it->labels[i]);

	gcli_clear_ptr(&it->labels);

	for (size_t i = 0; i < it->assignees_size; ++i)
		gcli_clear_ptr(&it->assignees[i]);

	gcli_clear_ptr(&it->assignees);
	gcli_clear_ptr(&it->milestone);
}

void
gcli_issues_free(struct gcli_issue_list *const list)
{
	for (size_t i = 0; i < list->issues_size; ++i)
		gcli_issue_free(&list->issues[i]);

	gcli_clear_ptr(&list->issues);
	list->issues_size = 0;
}

int
gcli_issues_search(struct gcli_ctx *ctx, struct gcli_path const *const path,
                   struct gcli_issue_fetch_details const *details, int const max,
                   struct gcli_issue_list *const out)
{
	gcli_null_check_call(search_issues, ctx, path, details, max, out);
}

int
gcli_get_issue(struct gcli_ctx *ctx, struct gcli_path const *const path,
               struct gcli_issue *const out)
{
	gcli_null_check_call(get_issue_summary, ctx, path, out);
}

int
gcli_issue_close(struct gcli_ctx *ctx, struct gcli_path const *const path)
{
	gcli_null_check_call(issue_close, ctx, path);
}

int
gcli_issue_reopen(struct gcli_ctx *ctx, struct gcli_path const *const path)
{
	gcli_null_check_call(issue_reopen, ctx, path);
}

int
gcli_issue_submit(struct gcli_ctx *ctx, struct gcli_submit_issue_options *opts)
{
	gcli_null_check_call(perform_submit_issue, ctx, opts, NULL);
}

int
gcli_issue_assign(struct gcli_ctx *ctx, struct gcli_path const *issue_path,
                  char const *assignee)
{
	gcli_null_check_call(issue_assign, ctx, issue_path, assignee);
}

int
gcli_issue_add_labels(struct gcli_ctx *ctx,
                      struct gcli_path const *const issue_path,
                      char const *const labels[], size_t const labels_size)
{
	gcli_null_check_call(issue_add_labels, ctx, issue_path, labels,
	                     labels_size);
}

int
gcli_issue_remove_labels(struct gcli_ctx *ctx,
                         struct gcli_path const *const issue_path,
                         char const *const labels[], size_t const labels_size)
{
	gcli_null_check_call(issue_remove_labels, ctx, issue_path, labels,
	                     labels_size);
}

int
gcli_issue_set_milestone(struct gcli_ctx *ctx,
                         struct gcli_path const *const issue_path,
                         int const milestone)
{
	gcli_null_check_call(issue_set_milestone, ctx, issue_path, milestone);
}

int
gcli_issue_clear_milestone(struct gcli_ctx *ctx,
                           struct gcli_path const *const issue_path)
{
	gcli_null_check_call(issue_clear_milestone, ctx, issue_path);
}

int
gcli_issue_set_title(struct gcli_ctx *ctx,
                     struct gcli_path const *const issue_path,
                     char const *new_title)
{
	gcli_null_check_call(issue_set_title, ctx, issue_path, new_title);
}

int
gcli_issue_get_attachments(struct gcli_ctx *ctx,
                           struct gcli_path const *const issue_path,
                           struct gcli_attachment_list *out)
{
	gcli_null_check_call(get_issue_attachments, ctx, issue_path, out);
}

int
gcli_issue_set_op(struct gcli_ctx *ctx,
                  struct gcli_path const *const issue_path,
                  char const *new_op)
{
	gcli_null_check_call(issue_set_op, ctx, issue_path, new_op);
}
