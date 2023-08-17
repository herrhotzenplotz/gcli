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

#include <gcli/cmd/colour.h>
#include <gcli/cmd/table.h>
#include <gcli/config.h>
#include <gcli/editor.h>
#include <gcli/forges.h>
#include <gcli/github/checks.h>
#include <gcli/github/pulls.h>
#include <gcli/gitlab/pipelines.h>
#include <gcli/json_util.h>
#include <gcli/pulls.h>
#include <sn/sn.h>

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

void
gcli_pull_get_diff(gcli_ctx *ctx, FILE *stream, char const *owner,
                   char const *reponame, int const pr_number)
{
	gcli_forge(ctx)->print_pull_diff(ctx, stream, owner, reponame, pr_number);
}

static int
gcli_get_pull_commits(gcli_ctx *ctx, char const *owner, char const *repo,
                      int const pr_number, gcli_commit_list *const out)
{
	return gcli_forge(ctx)->get_pull_commits(ctx, owner, repo, pr_number, out);
}

/**
 * Get a copy of the first line of the passed string.
 */
static char *
cut_newline(char const *const _it)
{
	char *it = strdup(_it);
	char *foo = it;
	while (*foo) {
		if (*foo == '\n') {
			*foo = 0;
			break;
		}
		foo += 1;
	}

	return it;
}

static void
gcli_print_commits_table(gcli_ctx *ctx, gcli_commit_list const *const list)
{
	gcli_tbl table;
	gcli_tblcoldef cols[] = {
		{ .name = "SHA",     .type = GCLI_TBLCOLTYPE_STRING, .flags = GCLI_TBLCOL_COLOUREXPL },
		{ .name = "AUTHOR",  .type = GCLI_TBLCOLTYPE_STRING, .flags = GCLI_TBLCOL_BOLD },
		{ .name = "EMAIL",   .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "DATE",    .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "MESSAGE", .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
	};

	(void) ctx;

	if (list->commits_size == 0) {
		puts("No commits");
		return;
	}

	table = gcli_tbl_begin(cols, ARRAY_SIZE(cols));
	if (!table)
		errx(1, "error: could not initialize table");

	for (size_t i = 0; i < list->commits_size; ++i) {
		char *message = cut_newline(list->commits[i].message);
		gcli_tbl_add_row(table, GCLI_COLOR_YELLOW, list->commits[i].sha,
		                 list->commits[i].author,
		                 list->commits[i].email,
		                 list->commits[i].date,
		                 message);
		free(message);          /* message is copied by the function above */
	}

	gcli_tbl_end(table);
}

static void
gcli_commits_free(gcli_commit_list *list)
{
	for (size_t i = 0; i < list->commits_size; ++i) {
		free(list->commits[i].sha);
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
gcli_pull_commits(gcli_ctx *ctx, char const *owner, char const *repo,
                  int const pr_number)
{
	gcli_commit_list commits = {0};

	if (gcli_get_pull_commits(ctx, owner, repo, pr_number, &commits) < 0)
		errx(1, "error: failed to fetch commits of the pull request");

	gcli_print_commits_table(ctx, &commits);
	gcli_commits_free(&commits);
}

void
gcli_pull_free(gcli_pull *const it)
{
	free(it->author);
	free(it->state);
	free(it->title);
	free(it->body);
	free(it->created_at);
	free(it->head_label);
	free(it->base_label);
	free(it->head_sha);

	for (size_t i = 0; i < it->labels_size; ++i)
		free(it->labels[i].data);

	free(it->labels);
}

int
gcli_get_pull(gcli_ctx *ctx, char const *owner, char const *repo,
              int const pr_number, gcli_pull *const out)
{
	return gcli_forge(ctx)->get_pull(ctx, owner, repo, pr_number, out);
}

/* This one is going to be nasty */
int
gcli_pull_checks(gcli_ctx *ctx, char const *owner, char const *repo, int const pr_number)
{
	return gcli_forge(ctx)->print_pull_checks(ctx, owner, repo, pr_number);
}

static void
pull_init_user_file(gcli_ctx *ctx, FILE *stream, void *_opts)
{
	gcli_submit_pull_options *opts = _opts;

	(void) ctx;
	fprintf(
		stream,
		"! PR TITLE : "SV_FMT"\n"
		"! Enter PR comments above.\n"
		"! All lines starting with '!' will be discarded.\n",
		SV_ARGS(opts->title));
}

static sn_sv
gcli_pull_get_user_message(gcli_ctx *ctx, gcli_submit_pull_options *opts)
{
	return gcli_editor_get_user_message(ctx, pull_init_user_file, opts);
}

int
gcli_pull_submit(gcli_ctx *ctx, gcli_submit_pull_options opts)
{
	opts.body = gcli_pull_get_user_message(ctx, &opts);

	fprintf(stdout,
	        "The following PR will be created:\n"
	        "\n"
	        "TITLE   : "SV_FMT"\n"
	        "BASE    : "SV_FMT"\n"
	        "HEAD    : "SV_FMT"\n"
	        "IN      : %s/%s\n"
	        "MESSAGE :\n"SV_FMT"\n",
	        SV_ARGS(opts.title),SV_ARGS(opts.to),
	        SV_ARGS(opts.from),
	        opts.owner, opts.repo,
	        SV_ARGS(opts.body));

	fputc('\n', stdout);

	if (!opts.always_yes)
		if (!sn_yesno("Do you want to continue?"))
			errx(1, "PR aborted.");

	return gcli_forge(ctx)->perform_submit_pull(ctx, opts);
}

int
gcli_pull_merge(gcli_ctx *ctx, char const *owner, char const *reponame,
                int const pr_number, enum gcli_merge_flags flags)
{
	return gcli_forge(ctx)->pull_merge(ctx, owner, reponame, pr_number, flags);
}

int
gcli_pull_close(gcli_ctx *ctx, char const *owner, char const *reponame,
                int const pr_number)
{
	return gcli_forge(ctx)->pull_close(ctx, owner, reponame, pr_number);
}

int
gcli_pull_reopen(gcli_ctx *ctx, char const *owner, char const *reponame,
                 int const pr_number)
{
	return gcli_forge(ctx)->pull_reopen(ctx, owner, reponame, pr_number);
}

int
gcli_pull_add_labels(gcli_ctx *ctx, char const *owner, char const *repo,
                     int const pr_number, char const *const labels[],
                     size_t const labels_size)
{
	return gcli_forge(ctx)->pull_add_labels(
		ctx, owner, repo, pr_number, labels, labels_size);
}

int
gcli_pull_remove_labels(gcli_ctx *ctx, char const *owner, char const *repo,
                        int const pr_number, char const *const labels[],
                        size_t const labels_size)
{
	return gcli_forge(ctx)->pull_remove_labels(
		ctx, owner, repo, pr_number, labels, labels_size);
}

int
gcli_pull_set_milestone(gcli_ctx *ctx, char const *owner, char const *repo,
                        int pr_number, int milestone_id)
{
	return gcli_forge(ctx)->pull_set_milestone(
		ctx, owner, repo, pr_number, milestone_id);
}

int
gcli_pull_clear_milestone(gcli_ctx *ctx, char const *owner, char const *repo,
                          int pr_number)
{
	return gcli_forge(ctx)->pull_clear_milestone(ctx, owner, repo, pr_number);
}
