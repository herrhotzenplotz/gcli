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

#include <gcli/cmd/colour.h>
#include <gcli/cmd/table.h>
#include <gcli/editor.h>
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

void
gcli_print_issues_table(gcli_ctx *ctx, enum gcli_output_flags const flags,
                        gcli_issue_list const *const list, int const max)
{
	int n, pruned = 0;
	gcli_tbl table;
	gcli_tblcoldef cols[] = {
		{ .name = "NUMBER", .type = GCLI_TBLCOLTYPE_INT, .flags = GCLI_TBLCOL_JUSTIFYR },
		{ .name = "NOTES",  .type = GCLI_TBLCOLTYPE_INT, .flags = GCLI_TBLCOL_JUSTIFYR },
		{ .name = "STATE",  .type = GCLI_TBLCOLTYPE_SV,  .flags = GCLI_TBLCOL_STATECOLOURED },
		{ .name = "TITLE",  .type = GCLI_TBLCOLTYPE_SV,  .flags = 0 },
	};

	(void) ctx;

	if (list->issues_size == 0) {
		puts("No issues");
		return;
	}

	table = gcli_tbl_begin(cols, ARRAY_SIZE(cols));
	if (!table)
		errx(1, "could not init table printer");

	/* Determine the correct number of items to print */
	if (max < 0 || (size_t)(max) > list->issues_size)
		n = list->issues_size;
	else
		n = max;

	/* Iterate depending on the output order */
	if (flags & OUTPUT_SORTED) {
		for (int i = 0; i < n; ++i) {
			if (!list->issues[n - 1 - 1].is_pr) {
				gcli_tbl_add_row(table,
				                 list->issues[n - i - 1].number,
				                 list->issues[n - i - 1].comments,
				                 list->issues[n - i - 1].state,
				                 list->issues[n - i - 1].title);
			} else {
				pruned++;
			}
		}
	} else {
		for (int i = 0; i < n; ++i) {
			if (!list->issues[i].is_pr) {
				gcli_tbl_add_row(table,
				                 list->issues[i].number,
				                 list->issues[i].comments,
				                 list->issues[i].state,
				                 list->issues[i].title);
			} else {
				pruned++;
			}
		}
	}

	/* Dump the table */
	gcli_tbl_end(table);

	/* Inform the user that we pruned pull requests from the output */
	if (pruned && sn_getverbosity() != VERBOSITY_QUIET)
		fprintf(stderr, "info: %d pull requests pruned\n", pruned);
}

void
gcli_issue_print_summary(gcli_ctx *ctx, gcli_issue const *const it)
{
	gcli_dict dict;

	(void) ctx;

	dict = gcli_dict_begin();

	gcli_dict_add(dict, "NAME", 0, 0, "%d", it->number);
	gcli_dict_add(dict, "TITLE", 0, 0, SV_FMT, SV_ARGS(it->title));
	gcli_dict_add(dict, "CREATED", 0, 0, SV_FMT, SV_ARGS(it->created_at));
	gcli_dict_add(dict, "AUTHOR",  GCLI_TBLCOL_BOLD, 0,
	              SV_FMT, SV_ARGS(it->author));
	gcli_dict_add(dict, "STATE", GCLI_TBLCOL_STATECOLOURED, 0,
	              SV_FMT, SV_ARGS(it->state));
	gcli_dict_add(dict, "COMMENTS", 0, 0, "%d", it->comments);
	gcli_dict_add(dict, "LOCKED", 0, 0, "%s", sn_bool_yesno(it->locked));

	if (it->milestone.length)
		gcli_dict_add(dict, "MILESTONE", 0, 0, SV_FMT, SV_ARGS(it->milestone));

	if (it->labels_size) {
		gcli_dict_add_sv_list(dict, "LABELS", it->labels, it->labels_size);
	} else {
		gcli_dict_add(dict, "LABELS", 0, 0, "none");
	}

	if (it->assignees_size) {
		gcli_dict_add_sv_list(dict, "ASSIGNEES",
		                      it->assignees, it->assignees_size);
	} else {
		gcli_dict_add(dict, "ASSIGNEES", 0, 0, "none");
	}

	/* Dump the dictionary */
	gcli_dict_end(dict);

}

void
gcli_issue_print_op(gcli_issue const *const it)
{
	if (it->body.length && it->body.data)
		pretty_print(it->body.data, 4, 80, stdout);
}

int
gcli_get_issue(gcli_ctx *ctx, char const *owner, char const *repo,
               int const issue_number, gcli_issue *const out)
{
	return gcli_forge(ctx)->get_issue_summary(
		ctx, owner, repo, issue_number, out);
}

int
gcli_issue_close(gcli_ctx *ctx, char const *owner, char const *repo,
                 int const issue_number)
{
	return gcli_forge(ctx)->issue_close(ctx, owner, repo, issue_number);
}

int
gcli_issue_reopen(gcli_ctx *ctx, char const *owner, char const *repo,
                  int const issue_number)
{
	return gcli_forge(ctx)->issue_reopen(ctx, owner, repo, issue_number);
}

static void
issue_init_user_file(gcli_ctx *ctx, FILE *stream, void *_opts)
{
	(void) ctx;
	gcli_submit_issue_options *opts = _opts;
	fprintf(
		stream,
		"! ISSUE TITLE : "SV_FMT"\n"
		"! Enter issue description above.\n"
		"! All lines starting with '!' will be discarded.\n",
		SV_ARGS(opts->title));
}

static sn_sv
gcli_issue_get_user_message(gcli_ctx *ctx, gcli_submit_issue_options *opts)
{
	return gcli_editor_get_user_message(ctx, issue_init_user_file, opts);
}

int
gcli_issue_submit(gcli_ctx *ctx, gcli_submit_issue_options opts)
{
	gcli_fetch_buffer json_buffer = {0};
	int rc = 0;

	opts.body = gcli_issue_get_user_message(ctx, &opts);

	printf("The following issue will be created:\n"
	       "\n"
	       "TITLE   : "SV_FMT"\n"
	       "OWNER   : %s\n"
	       "REPO    : %s\n"
	       "MESSAGE :\n"SV_FMT"\n",
	       SV_ARGS(opts.title),
	       opts.owner, opts.repo,
	       SV_ARGS(opts.body));

	putchar('\n');

	if (!opts.always_yes) {
		if (!sn_yesno("Do you want to continue?"))
			errx(1, "Submission aborted.");
	}

	rc = gcli_forge(ctx)->perform_submit_issue(ctx, opts, &json_buffer);

	if (rc == 0)
		gcli_print_html_url(ctx, json_buffer);

	free(opts.body.data);
	free(opts.body.data);
	free(json_buffer.data);

	return rc;
}

int
gcli_issue_assign(gcli_ctx *ctx, char const *owner, char const *repo,
                  int const issue_number, char const *assignee)
{
	return gcli_forge(ctx)->issue_assign(ctx, owner, repo, issue_number, assignee);
}

int
gcli_issue_add_labels(gcli_ctx *ctx, char const *owner, char const *repo,
                      int const issue, char const *const labels[],
                      size_t const labels_size)
{
	return gcli_forge(ctx)->issue_add_labels(ctx,owner, repo, issue, labels,
	                                         labels_size);
}

int
gcli_issue_remove_labels(gcli_ctx *ctx, char const *owner, char const *repo,
                         int const issue, char const *const labels[],
                         size_t const labels_size)
{
	return gcli_forge(ctx)->issue_remove_labels(
		ctx, owner, repo, issue, labels, labels_size);
}

int
gcli_issue_set_milestone(gcli_ctx *ctx, char const *const owner,
                         char const *const repo, int const issue,
                         int const milestone)
{
	return gcli_forge(ctx)->issue_set_milestone(
		ctx, owner, repo, issue, milestone);
}

int
gcli_issue_clear_milestone(gcli_ctx *ctx, char const *const owner,
                           char const *const repo, int const issue)
{
	return gcli_forge(ctx)->issue_clear_milestone(ctx, owner, repo, issue);
}
