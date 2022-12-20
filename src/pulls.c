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

#include <gcli/colour.h>
#include <gcli/config.h>
#include <gcli/editor.h>
#include <gcli/forges.h>
#include <gcli/github/checks.h>
#include <gcli/github/pulls.h>
#include <gcli/gitlab/pipelines.h>
#include <gcli/json_util.h>
#include <gcli/pulls.h>
#include <gcli/table.h>
#include <sn/sn.h>

void
gcli_pulls_free(gcli_pull *const it, int const n)
{
	for (int i = 0; i < n; ++i) {
		free((void *)it[i].title);
		free((void *)it[i].state);
		free((void *)it[i].creator);
	}
}

int
gcli_get_prs(char const *owner,
             char const *repo,
             bool const all,
             int const max,
             gcli_pull **const out)
{
	return gcli_forge()->get_prs(owner, repo, all, max, out);
}

void
gcli_print_pr_table(enum gcli_output_flags const flags,
                    gcli_pull const *const pulls,
                    int const pulls_size)
{
	gcli_tbl table;
	gcli_tblcoldef cols[] = {
		{ .name = "NUMBER",  .type = GCLI_TBLCOLTYPE_INT,    .flags = GCLI_TBLCOL_JUSTIFYR },
		{ .name = "STATE",   .type = GCLI_TBLCOLTYPE_STRING, .flags = GCLI_TBLCOL_STATECOLOURED },
		{ .name = "MERGED",  .type = GCLI_TBLCOLTYPE_BOOL,   .flags = 0 },
		{ .name = "CREATOR", .type = GCLI_TBLCOLTYPE_STRING, .flags = GCLI_TBLCOL_BOLD },
		{ .name = "TITLE",   .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
	};

	if (pulls_size == 0) {
		puts("No Pull Requests");
		return;
	}


	table = gcli_tbl_begin(cols, ARRAY_SIZE(cols));
	if (!table)
		errx(1, "error: cannot init table");

	if (flags & OUTPUT_SORTED) {
		for (int i = pulls_size; i > 0; --i) {
			gcli_tbl_add_row(table, pulls[i-1].number, pulls[i-1].state,
			                 pulls[i-1].merged, pulls[i-1].creator,
			                 pulls[i-1].title);
		}
	} else {
		for (int i = 0; i < pulls_size; ++i) {
			gcli_tbl_add_row(table, pulls[i].number, pulls[i].state,
			                 pulls[i].merged, pulls[i].creator, pulls[i].title);
		}
	}

	gcli_tbl_end(table);
}

void
gcli_print_pr_diff(FILE *stream,
                   char const *owner,
                   char const *reponame,
                   int const pr_number)
{
	gcli_forge()->print_pr_diff(stream, owner, reponame, pr_number);
}

static void
gcli_print_pr_summary(gcli_pull_summary const *const it)
{
	gcli_dict dict;
	gcli_forge_descriptor const *const forge = gcli_forge();

	dict = gcli_dict_begin();

	gcli_dict_add(dict,            "NUMBER", 0, 0, "%d", it->number);
	gcli_dict_add_string(dict,     "TITLE", 0, 0, it->title);
	gcli_dict_add_string(dict,     "HEAD", 0, 0, it->head_label);
	gcli_dict_add_string(dict,     "BASE", 0, 0, it->base_label);
	gcli_dict_add_string(dict,     "CREATED", 0, 0, it->created_at);
	gcli_dict_add_string(dict,     "AUTHOR", GCLI_TBLCOL_BOLD, 0, it->author);
	gcli_dict_add_string(dict,     "STATE", GCLI_TBLCOL_STATECOLOURED, 0, it->state);
	gcli_dict_add(dict,            "COMMENTS", 0, 0, "%d", it->comments);

	if (!(forge->pull_summary_quirks & GCLI_PRS_QUIRK_ADDDEL))
		/* FIXME: move printing colours into the dictionary printer? */
		gcli_dict_add(dict,        "ADD:DEL", 0, 0, "%s%d%s:%s%d%s",
		              gcli_setcolour(GCLI_COLOR_GREEN),
		              it->additions,
		              gcli_resetcolour(),
		              gcli_setcolour(GCLI_COLOR_RED),
		              it->deletions,
		              gcli_resetcolour());

	if (!(forge->pull_summary_quirks & GCLI_PRS_QUIRK_COMMITS))
		gcli_dict_add(dict,        "COMMITS", 0, 0, "%d", it->commits);

	if (!(forge->pull_summary_quirks & GCLI_PRS_QUIRK_CHANGES))
		gcli_dict_add(dict,        "CHANGED", 0, 0, "%d", it->changed_files);

	if (!(forge->pull_summary_quirks & GCLI_PRS_QUIRK_MERGED))
		gcli_dict_add_string(dict, "MERGED", 0, 0, sn_bool_yesno(it->merged));

	gcli_dict_add_string(dict,     "MERGEABLE", 0, 0, sn_bool_yesno(it->mergeable));
	gcli_dict_add_string(dict,     "DRAFT", 0, 0, sn_bool_yesno(it->draft));

	if (it->labels_size) {
		gcli_dict_add_sv_list(dict, "LABELS", it->labels, it->labels_size);
	} else {
		gcli_dict_add_string(dict, "LABELS", 0, 0, "none");
	}

	gcli_dict_end(dict);

	if (it->body) {
		putchar('\n');
		pretty_print(it->body, 4, 80, stdout);
	}
}

static int
gcli_get_pull_commits(char const *owner,
                      char const *repo,
                      int const pr_number,
                      gcli_commit **const out)
{
	return gcli_forge()->get_pull_commits(owner, repo, pr_number, out);
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
gcli_print_commits_table(gcli_commit const *const commits,
                         int const commits_size)
{
	gcli_tbl table;
	gcli_tblcoldef cols[] = {
		{ .name = "SHA",     .type = GCLI_TBLCOLTYPE_STRING, .flags = GCLI_TBLCOL_COLOUREXPL },
		{ .name = "AUTHOR",  .type = GCLI_TBLCOLTYPE_STRING, .flags = GCLI_TBLCOL_BOLD },
		{ .name = "EMAIL",   .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "DATE",    .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "MESSAGE", .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
	};

	if (commits_size == 0) {
		puts("No commits");
		return;
	}

	table = gcli_tbl_begin(cols, ARRAY_SIZE(cols));
	if (!table)
		errx(1, "error: could not initialize table");

	for (int i = 0; i < commits_size; ++i) {
		char *message = cut_newline(commits[i].message);
		gcli_tbl_add_row(table, GCLI_COLOR_YELLOW, commits[i].sha,
		                 commits[i].author, commits[i].email, commits[i].date,
		                 message);
		free(message);          /* message is copied by the function above */
	}

	gcli_tbl_end(table);
}

static void
gcli_commits_free(gcli_commit *it, int const size)
{
	for (int i = 0; i < size; ++i) {
		free((void *)it[i].sha);
		free((void *)it[i].message);
		free((void *)it[i].date);
		free((void *)it[i].author);
		free((void *)it[i].email);
	}

	free(it);
}

void
gcli_pulls_summary_free(gcli_pull_summary *const it)
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
}

static void
gcli_get_pull_summary(char const *owner,
                      char const *repo,
                      int const pr_number,
                      gcli_pull_summary *const out)
{
	gcli_forge()->get_pull_summary(owner, repo, pr_number, out);
}

static void
gcli_pr_info(char const *owner,
             char const *repo,
             int const pr_number,
             bool const is_status)
{
	gcli_pull_summary  summary      = {0};
	gcli_commit       *commits      = NULL;
	int                commits_size = 0;

	/* Summary header */
	gcli_get_pull_summary(owner, repo, pr_number, &summary);
	gcli_print_pr_summary(&summary);

	/* Commits */
	commits_size = gcli_get_pull_commits(owner, repo, pr_number, &commits);

	puts("\nCOMMITS");
	gcli_print_commits_table(commits, commits_size);

	gcli_commits_free(commits, commits_size);

	/* Only print checks if the user issued the 'status' action */
	if (is_status) {
		switch (gcli_config_get_forge_type()) {
		case GCLI_FORGE_GITHUB:
		case GCLI_FORGE_GITLAB:
			puts("\nCHECKS");
			gcli_pr_checks(owner, repo, pr_number);
			break;
		default:
			break;
		}
	}

	gcli_pulls_summary_free(&summary);
}

void
gcli_pr_status(char const *owner,
               char const *repo,
               int const pr_number)
{
	gcli_pr_info(owner, repo, pr_number, true);
}

void
gcli_pr_summary(char const *owner,
                char const *repo,
                int const pr_number)
{
	gcli_pr_info(owner, repo, pr_number, false);
}

void
gcli_pr_checks(char const *owner, char const *repo, int const pr_number)
{
	gcli_forge()->print_pr_checks(owner, repo, pr_number);
}

static void
pr_init_user_file(FILE *stream, void *_opts)
{
	gcli_submit_pull_options *opts = _opts;
	fprintf(
		stream,
		"! PR TITLE : "SV_FMT"\n"
		"! Enter PR comments above.\n"
		"! All lines starting with '!' will be discarded.\n",
		SV_ARGS(opts->title));
}

static sn_sv
gcli_pr_get_user_message(gcli_submit_pull_options *opts)
{
	return gcli_editor_get_user_message(pr_init_user_file, opts);
}

void
gcli_pr_submit(gcli_submit_pull_options opts)
{
	opts.body = gcli_pr_get_user_message(&opts);

	fprintf(stdout,
	        "The following PR will be created:\n"
	        "\n"
	        "TITLE   : "SV_FMT"\n"
	        "BASE    : "SV_FMT"\n"
	        "HEAD    : "SV_FMT"\n"
	        "IN      : "SV_FMT"/"SV_FMT"\n"
	        "MESSAGE :\n"SV_FMT"\n",
	        SV_ARGS(opts.title),SV_ARGS(opts.to),
	        SV_ARGS(opts.from),
	        SV_ARGS(opts.owner), SV_ARGS(opts.repo),
	        SV_ARGS(opts.body));

	fputc('\n', stdout);

	if (!opts.always_yes)
		if (!sn_yesno("Do you want to continue?"))
			errx(1, "PR aborted.");

	gcli_forge()->perform_submit_pr(opts);
}

void
gcli_pr_merge(char const *owner,
              char const *reponame,
              int const pr_number,
              bool const squash)
{
	gcli_forge()->pr_merge(owner, reponame, pr_number, squash);
}

void
gcli_pr_close(char const *owner, char const *reponame, int const pr_number)
{
	gcli_forge()->pr_close(owner, reponame, pr_number);
}

void
gcli_pr_reopen(char const *owner, char const *reponame, int const pr_number)
{
	gcli_forge()->pr_reopen(owner, reponame, pr_number);
}

void
gcli_pr_add_labels(char const *owner,
                   char const *repo,
                   int const pr_number,
                   char const *const labels[],
                   size_t const labels_size)
{
	gcli_forge()->pr_add_labels(
		owner, repo, pr_number, labels, labels_size);
}

void
gcli_pr_remove_labels(char const *owner,
                      char const *repo,
                      int const pr_number,
                      char const *const labels[],
                      size_t const labels_size)
{
	gcli_forge()->pr_remove_labels(
		owner, repo, pr_number, labels, labels_size);
}
