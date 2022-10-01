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

#include <gcli/color.h>
#include <gcli/config.h>
#include <gcli/editor.h>
#include <gcli/forges.h>
#include <gcli/github/pulls.h>
#include <gcli/github/checks.h>
#include <gcli/json_util.h>
#include <gcli/pulls.h>
#include <sn/sn.h>

void
gcli_pulls_free(gcli_pull *it, int n)
{
    for (int i = 0; i < n; ++i) {
        free((void *)it[i].title);
        free((void *)it[i].state);
        free((void *)it[i].creator);
    }
}

int
gcli_get_prs(
    const char  *owner,
    const char  *repo,
    bool         all,
    int          max,
    gcli_pull  **out)
{
    return gcli_forge()->get_prs(owner, repo, all, max, out);
}

void
gcli_print_pr_table(
    enum gcli_output_order  order,
    gcli_pull              *pulls,
    int                     pulls_size)
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
                   gcli_state_color_str(pulls[i - 1].state),
                   pulls[i - 1].state,
                   gcli_resetcolor(),
                   sn_bool_yesno(pulls[i - 1].merged),
                   gcli_setbold(),
                   pulls[i - 1].creator,
                   gcli_resetbold(),
                   pulls[i - 1].title);
        }
    } else {
        for (int i = 0; i < pulls_size; ++i) {
            printf("%6d  %s%6.6s%s  %6.6s  %s%20.20s%s  %-s\n",
                   pulls[i].number,
                   gcli_state_color_str(pulls[i].state),
                   pulls[i].state,
                   gcli_resetcolor(),
                   sn_bool_yesno(pulls[i].merged),
                   gcli_setbold(),
                   pulls[i].creator,
                   gcli_resetbold(),
                   pulls[i].title);
        }
    }
}

void
gcli_print_pr_diff(
    FILE       *stream,
    const char *owner,
    const char *reponame,
    int         pr_number)
{
    gcli_forge()->print_pr_diff(stream, owner, reponame, pr_number);
}

static void
gcli_print_pr_summary(gcli_pull_summary *it)
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
           gcli_setbold(), SANITIZE(it->author), gcli_resetbold(),
           gcli_state_color_str(it->state), SANITIZE(it->state), gcli_resetcolor(),
           it->comments,
           gcli_setcolor(GCLI_COLOR_GREEN), it->additions, gcli_resetcolor(),
           gcli_setcolor(GCLI_COLOR_RED),   it->deletions, gcli_resetcolor(),
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
gcli_get_pull_commits(
    const char   *owner,
    const char   *repo,
    int           pr_number,
    gcli_commit **out)
{
    return gcli_forge()->get_pull_commits(owner, repo, pr_number, out);
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
gcli_print_commits_table(gcli_commit *commits, int commits_size)
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
               gcli_setcolor(GCLI_COLOR_YELLOW),
               commits[i].sha,
               gcli_resetcolor(),
               gcli_setbold(),
               commits[i].author,
               gcli_resetbold(),
               commits[i].email,
               commits[i].date,
               message);
        free(message);
    }
}

static void
gcli_commits_free(gcli_commit *it, int size)
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
gcli_pulls_summary_free(gcli_pull_summary *it)
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

    for (size_t i = 0; i < it->labels_size; ++i)
        free(it->labels[i].data);
}

static void
gcli_get_pull_summary(
    const char        *owner,
    const char        *repo,
    int                pr_number,
    gcli_pull_summary *out)
{
    gcli_forge()->get_pull_summary(owner, repo, pr_number, out);
}

static void
gcli_pr_info(
    const char *owner,
    const char *repo,
    int         pr_number,
    bool        is_status)
{
    gcli_pull_summary  summary      = {0};
    gcli_commit       *commits      = NULL;
    int                 commits_size = 0;

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
        if (gcli_config_get_forge_type() == GCLI_FORGE_GITHUB) {
            puts("\nCHECKS");
            github_checks(owner, repo, summary.head_sha, -1);
        }
    }

    gcli_pulls_summary_free(&summary);
}

void
gcli_pr_status(
    const char *owner,
    const char *repo,
    int         pr_number)
{
    gcli_pr_info(owner, repo, pr_number, true);
}

void
gcli_pr_summary(
    const char *owner,
    const char *repo,
    int         pr_number)
{
    gcli_pr_info(owner, repo, pr_number, false);
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
gcli_pr_merge(
    const char *owner,
    const char *reponame,
    int         pr_number,
    bool        squash)
{
    gcli_forge()->pr_merge(owner, reponame, pr_number, squash);
}

void
gcli_pr_close(const char *owner, const char *reponame, int pr_number)
{
    gcli_forge()->pr_close(owner, reponame, pr_number);
}

void
gcli_pr_reopen(const char *owner, const char *reponame, int pr_number)
{
    gcli_forge()->pr_reopen(owner, reponame, pr_number);
}

void
gcli_pr_add_labels(
    const char *owner,
    const char *repo,
    int         pr_number,
    const char *labels[],
    size_t      labels_size)
{
    gcli_forge()->pr_add_labels(
        owner, repo, pr_number, labels, labels_size);
}

void
gcli_pr_remove_labels(
    const char *owner,
    const char *repo,
    int         pr_number,
    const char *labels[],
    size_t      labels_size)
{
    gcli_forge()->pr_remove_labels(
        owner, repo, pr_number, labels, labels_size);
}
