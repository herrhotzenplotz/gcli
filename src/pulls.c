/*
 * Copyright 2021 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <ghcli/color.h>
#include <ghcli/editor.h>
#include <ghcli/forges.h>
#include <ghcli/github/pulls.h>
#include <ghcli/json_util.h>
#include <ghcli/pulls.h>
#include <sn/sn.h>

void
ghcli_pulls_free(ghcli_pull *it, int n)
{
	for (int i = 0; i < n; ++i) {
		free((void *)it[i].title);
		free((void *)it[i].state);
		free((void *)it[i].creator);
	}
}

int
ghcli_get_prs(
	const char  *owner,
	const char  *repo,
	bool         all,
	int          max,
	ghcli_pull **out)
{
	return ghcli_forge()->get_prs(owner, repo, all, max, out);
}

void
ghcli_print_pr_table(
	enum ghcli_output_order  order,
	ghcli_pull              *pulls,
	int                      pulls_size)
{
	if (pulls_size == 0) {
		puts("No Pull Requests");
		return;
	}

	printf("%-6.6s  %6.6s  %6.6s  %20.20s  %-s\n",
	       "NUMBER", "STATE", "MERGED", "CREATOR", "TITLE");

	if (order == OUTPUT_ORDER_SORTED) {
		for (int i = pulls_size; i > 0; --i) {
			printf("%6d  %s%6.6s%s  %6.6s  %s%20.20s%s  %-s\n",
			       pulls[i - 1].number,
			       ghcli_state_color_str(pulls[i - 1].state),
			       pulls[i - 1].state,
			       ghcli_resetcolor(),
			       sn_bool_yesno(pulls[i - 1].merged),
			       ghcli_setbold(),
			       pulls[i - 1].creator,
			       ghcli_resetbold(),
			       pulls[i - 1].title);
		}
	} else {
		for (int i = 0; i < pulls_size; ++i) {
			printf("%6d  %s%6.6s%s  %6.6s  %s%20.20s%s  %-s\n",
			       pulls[i].number,
			       ghcli_state_color_str(pulls[i].state),
			       pulls[i].state,
			       ghcli_resetcolor(),
			       sn_bool_yesno(pulls[i].merged),
			       ghcli_setbold(),
			       pulls[i].creator,
			       ghcli_resetbold(),
			       pulls[i].title);
		}
	}
}

void
ghcli_print_pr_diff(
	FILE       *stream,
	const char *owner,
	const char *reponame,
	int         pr_number)
{
	ghcli_forge()->print_pr_diff(stream, owner, reponame, pr_number);
}

static void
ghcli_print_pr_summary(ghcli_pull_summary *it)
{
#define SANITIZE(x) (x ? x : "N/A")
	printf("   NUMBER : %d\n"
	       "    TITLE : %s\n"
	       "     HEAD : %s\n"
	       "     BASE : %s\n"
	       "  CREATED : %s\n"
	       "   AUTHOR : %s%s%s\n"
	       "    STATE : %s%s%s\n"
	       " COMMENTS : %d\n"
	       "  ADD:DEL : %s%d%s:%s%d%s\n"
	       "  COMMITS : %d\n"
	       "  CHANGED : %d\n"
	       "   MERGED : %s\n"
	       "MERGEABLE : %s\n"
	       "    DRAFT : %s\n"
	       "   LABELS : ",
	       it->number,
	       SANITIZE(it->title),
	       SANITIZE(it->head_label),
	       SANITIZE(it->base_label),
	       SANITIZE(it->created_at),
	       ghcli_setbold(), SANITIZE(it->author), ghcli_resetbold(),
	       ghcli_state_color_str(it->state), SANITIZE(it->state), ghcli_resetcolor(),
	       it->comments,
	       ghcli_setcolor(GHCLI_COLOR_GREEN), it->additions, ghcli_resetcolor(),
	       ghcli_setcolor(GHCLI_COLOR_RED),   it->deletions, ghcli_resetcolor(),
	       it->commits, it->changed_files,
	       sn_bool_yesno(it->merged),
	       sn_bool_yesno(it->mergeable),
	       sn_bool_yesno(it->draft));
#undef SANITIZE

	if (it->labels_size) {
		printf(SV_FMT, SV_ARGS(it->labels[0]));

		for (size_t i = 1; i < it->labels_size; ++i)
			printf(", "SV_FMT, SV_ARGS(it->labels[i]));
	} else {
		fputs("none", stdout);
	}
	fputs("\n\n", stdout);

	if (it->body)
		pretty_print(it->body, 4, 80, stdout);
}

static int
ghcli_get_pull_commits(
	const char    *owner,
	const char    *repo,
	int            pr_number,
	ghcli_commit **out)
{
	return ghcli_forge()->get_pull_commits(owner, repo, pr_number, out);
}

/**
 * Get a copy of the first line of the passed string.
 */
static char *
cut_newline(const char *_it)
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
ghcli_print_commits_table(ghcli_commit *commits, int commits_size)
{
	if (commits_size == 0) {
		puts("No commits");
		return;
	}

	printf("%-8.8s  %-15.15s  %-20.20s  %-16.16s  %-s\n",
	       "SHA", "AUTHOR", "EMAIL", "DATE", "MESSAGE");

	for (int i = 0; i < commits_size; ++i) {
		char *message = cut_newline(commits[i].message);
		printf("%s%-8.8s%s  %s%-15.15s%s  %-20.20s  %-16.16s  %-s\n",
		       ghcli_setcolor(GHCLI_COLOR_YELLOW),
		       commits[i].sha,
		       ghcli_resetcolor(),
		       ghcli_setbold(),
		       commits[i].author,
		       ghcli_resetbold(),
		       commits[i].email,
		       commits[i].date,
		       message);
		free(message);
	}
}

static void
ghcli_commits_free(ghcli_commit *it, int size)
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
ghcli_pulls_summary_free(ghcli_pull_summary *it)
{
	free((void *)it->author);
	free((void *)it->state);
	free((void *)it->title);
	free((void *)it->body);
	free((void *)it->created_at);
	free((void *)it->commits_link);

	for (size_t i = 0; i < it->labels_size; ++i)
		free(it->labels[i].data);
}

static void
ghcli_get_pull_summary(
	const char         *owner,
	const char         *repo,
	int                 pr_number,
	ghcli_pull_summary *out)
{
	ghcli_forge()->get_pull_summary(owner, repo, pr_number, out);
}

void
ghcli_pr_summary(
	const char *owner,
	const char *repo,
	int         pr_number)
{
	ghcli_pull_summary  summary      = {0};
	ghcli_commit       *commits      = NULL;
	int                 commits_size = 0;

	ghcli_get_pull_summary(owner, repo, pr_number, &summary);
	ghcli_print_pr_summary(&summary);
	ghcli_pulls_summary_free(&summary);

	/* Commits */
	commits_size = ghcli_get_pull_commits(owner, repo, pr_number, &commits);

	puts("\nCOMMITS");
	ghcli_print_commits_table(commits, commits_size);

	ghcli_commits_free(commits, commits_size);
}

static void
pr_init_user_file(FILE *stream, void *_opts)
{
	ghcli_submit_pull_options *opts = _opts;
	fprintf(
		stream,
		"# PR TITLE : "SV_FMT"\n"
		"# Enter PR comments below.\n"
		"# All lines starting with '#' will be discarded.\n",
		SV_ARGS(opts->title)
		);
}

static sn_sv
ghcli_pr_get_user_message(ghcli_submit_pull_options *opts)
{
	return ghcli_editor_get_user_message(pr_init_user_file, opts);
}

void
ghcli_pr_submit(ghcli_submit_pull_options opts)
{
	ghcli_fetch_buffer  json_buffer  = {0};

	sn_sv body = ghcli_pr_get_user_message(&opts);
	opts.body = ghcli_json_escape(body);

	fprintf(stdout,
		"The following PR will be created:\n"
		"\n"
		"TITLE   : "SV_FMT"\n"
		"BASE    : "SV_FMT"\n"
		"HEAD    : "SV_FMT"\n"
		"IN      : "SV_FMT"\n"
		"MESSAGE :\n"SV_FMT"\n",
		SV_ARGS(opts.title),SV_ARGS(opts.to),
		SV_ARGS(opts.from), SV_ARGS(opts.in),
		SV_ARGS(body));

	fputc('\n', stdout);

	if (!opts.always_yes)
		if (!sn_yesno("Do you want to continue?"))
			errx(1, "PR aborted.");

	ghcli_forge()->perform_submit_pr(opts, &json_buffer);

	ghcli_print_html_url(json_buffer);

	free(body.data);
	free(opts.body.data);
	free(json_buffer.data);
}

void
ghcli_pr_merge(
	const char *owner,
	const char *reponame,
	int         pr_number)
{
	ghcli_forge()->pr_merge(owner, reponame, pr_number);
}

void
ghcli_pr_close(const char *owner, const char *reponame, int pr_number)
{
	ghcli_forge()->pr_close(owner, reponame, pr_number);
}

void
ghcli_pr_reopen(const char *owner, const char *reponame, int pr_number)
{
	ghcli_forge()->pr_reopen(owner, reponame, pr_number);
}

void
ghcli_pr_add_labels(
	const char *owner,
	const char *repo,
	int         pr_number,
	const char *labels[],
	size_t      labels_size)
{
	ghcli_forge()->pr_add_labels(
		owner, repo, pr_number, labels, labels_size);
}

void
ghcli_pr_remove_labels(
	const char *owner,
	const char *repo,
	int         pr_number,
	const char *labels[],
	size_t      labels_size)
{
	ghcli_forge()->pr_remove_labels(
		owner, repo, pr_number, labels, labels_size);
}
