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

#include <gcli/colour.h>
#include <gcli/editor.h>
#include <gcli/forges.h>
#include <gcli/github/issues.h>
#include <gcli/issues.h>
#include <gcli/json_util.h>
#include <gcli/table.h>
#include <sn/sn.h>

static void
gcli_issue_free(gcli_issue *const it)
{
	free(it->title.data);
	free(it->created_at.data);
	free(it->author.data);
	free(it->state.data);
	free(it->body.data);

	for (size_t i = 0; i < it->labels_size; ++i)
		free(it->labels[i].data);

	free(it->labels);
}

void
gcli_issues_free(gcli_issue *issues, int const issues_size)
{
	for (int i = 0; i < issues_size; ++i)
		gcli_issue_free(&issues[i]);

	free(issues);
}

int
gcli_get_issues(char const *owner,
                char const *repo,
                bool const all,
                int const max,
                gcli_issue **const out)
{
	return gcli_forge()->get_issues(owner, repo, all, max, out);
}

void
gcli_print_issues_table(enum gcli_output_flags const flags,
                        gcli_issue const *const issues,
                        int const issues_size)
{
	gcli_tbl table;
	gcli_tblcoldef cols[] = {
		{ .name = "NUMBER", .type = GCLI_TBLCOLTYPE_INT, .flags = GCLI_TBLCOL_JUSTIFYR },
		{ .name = "STATE",  .type = GCLI_TBLCOLTYPE_SV,  .flags = GCLI_TBLCOL_STATECOLOURED },
		{ .name = "TITLE",  .type = GCLI_TBLCOLTYPE_SV,  .flags = 0 },
	};

	if (issues_size == 0) {
		puts("No issues");
		return;
	}

	table = gcli_tbl_begin(cols, ARRAY_SIZE(cols));
	if (!table)
		errx(1, "could not init table printer");

	if (flags & OUTPUT_SORTED) {
		for (int i = issues_size; i > 0; --i) {
			gcli_tbl_add_row(table, issues[i - 1].number, issues[i - 1].state,
			                 issues[i - 1].title);
		}
	} else {
		for (int i = 0; i < issues_size; ++i) {
			gcli_tbl_add_row(table, issues[i].number, issues[i].state,
			                 issues[i].title);
		}
	}

	gcli_tbl_end(table);
}

static void
gcli_print_issue_summary(gcli_issue const *const it)
{
	printf("   NUMBER : %d\n"
	       "    TITLE : "SV_FMT"\n"
	       "  CREATED : "SV_FMT"\n"
	       "   AUTHOR : %s"SV_FMT"%s\n"
	       "    STATE : %s"SV_FMT"%s\n"
	       " COMMENTS : %d\n"
	       "   LOCKED : %s\n"
	       "   LABELS : ",
	       it->number,
	       SV_ARGS(it->title), SV_ARGS(it->created_at),
	       gcli_setbold(), SV_ARGS(it->author), gcli_resetbold(),
	       gcli_state_colour_sv(it->state), SV_ARGS(it->state), gcli_resetcolour(),
	       it->comments, sn_bool_yesno(it->locked));

	if (it->labels_size) {
		printf(SV_FMT, SV_ARGS(it->labels[0]));

		for (size_t i = 1; i < it->labels_size; ++i)
			printf(", "SV_FMT, SV_ARGS(it->labels[i]));
	} else {
		printf("none");
	}

	putchar('\n');

	if (it->assignees_size) {
		printf("ASSIGNEES : "SV_FMT, SV_ARGS(it->assignees[0]));
		for (size_t i = 1; i < it->assignees_size; ++i)
			printf(", "SV_FMT, SV_ARGS(it->assignees[i]));
	} else {
		printf("ASSIGNEES : none\n");
	}

	putchar('\n');

	/* The API may not return a body if the user didn't put in any
	 * comment */
	if (it->body.data) {
		pretty_print(it->body.data, 4, 80, stdout);
		putchar('\n');
	}
}

void
gcli_issue_summary(char const *owner,
                   char const *repo,
                   int const issue_number)
{
	gcli_issue details = {0};

	gcli_forge()->get_issue_summary(owner, repo, issue_number, &details);
	gcli_print_issue_summary(&details);
	gcli_issue_free(&details);
}

void
gcli_issue_close(char const *owner, char const *repo, int const issue_number)
{
	gcli_forge()->issue_close(owner, repo, issue_number);
}

void
gcli_issue_reopen(char const *owner, char const *repo, int const issue_number)
{
	gcli_forge()->issue_reopen(owner, repo, issue_number);
}

static void
issue_init_user_file(FILE *stream, void *_opts)
{
	gcli_submit_issue_options *opts = _opts;
	fprintf(
		stream,
		"! ISSUE TITLE : "SV_FMT"\n"
		"! Enter issue description above.\n"
		"! All lines starting with '!' will be discarded.\n",
		SV_ARGS(opts->title));
}

static sn_sv
gcli_issue_get_user_message(gcli_submit_issue_options *opts)
{
	return gcli_editor_get_user_message(issue_init_user_file, opts);
}

void
gcli_issue_submit(gcli_submit_issue_options opts)
{
	gcli_fetch_buffer json_buffer = {0};

	opts.body = gcli_issue_get_user_message(&opts);

	printf("The following issue will be created:\n"
	       "\n"
	       "TITLE   : "SV_FMT"\n"
	       "OWNER   : "SV_FMT"\n"
	       "REPO    : "SV_FMT"\n"
	       "MESSAGE :\n"SV_FMT"\n",
	       SV_ARGS(opts.title), SV_ARGS(opts.owner),
	       SV_ARGS(opts.repo), SV_ARGS(opts.body));

	putchar('\n');

	if (!opts.always_yes) {
		if (!sn_yesno("Do you want to continue?"))
			errx(1, "Submission aborted.");
	}

	gcli_forge()->perform_submit_issue(opts, &json_buffer);
	gcli_print_html_url(json_buffer);

	free(opts.body.data);
	free(opts.body.data);
	free(json_buffer.data);
}

void
gcli_issue_assign(char const *owner,
                  char const *repo,
                  int const issue_number,
                  char const *assignee)
{
	gcli_forge()->issue_assign(owner, repo, issue_number, assignee);
}

void
gcli_issue_add_labels(char const *owner,
                      char const *repo,
                      int const issue,
                      char const *const labels[],
                      size_t const labels_size)
{
	gcli_forge()->issue_add_labels(owner, repo, issue, labels, labels_size);
}

void
gcli_issue_remove_labels(char const *owner,
                         char const *repo,
                         int const issue,
                         char const *const labels[],
                         size_t const labels_size)
{
	gcli_forge()->issue_remove_labels(owner, repo, issue, labels, labels_size);
}
