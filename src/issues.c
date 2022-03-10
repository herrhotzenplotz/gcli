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

#include <ghcli/forges.h>
#include <ghcli/editor.h>
#include <ghcli/github/issues.h>
#include <ghcli/issues.h>
#include <ghcli/json_util.h>
#include <sn/sn.h>

void
ghcli_issues_free(ghcli_issue *it, int size)
{
    for (int i = 0; i < size; ++i) {
        free((void *)it[i].title);
        free((void *)it[i].state);
    }

    free(it);
}

int
ghcli_get_issues(
    const char   *owner,
    const char   *repo,
    bool          all,
    int           max,
    ghcli_issue **out)
{
    return ghcli_forge()->get_issues(owner, repo, all, max, out);
}

void
ghcli_print_issues_table(
    FILE                    *stream,
    enum ghcli_output_order  order,
    ghcli_issue             *issues,
    int                      issues_size)
{
    if (issues_size == 0) {
        fprintf(stream, "No issues\n");
        return;
    }

    fprintf(stream, "%6.6s  %7.7s  %-s\n", "NUMBER", "STATE", "TITLE");

    if (order == OUTPUT_ORDER_SORTED) {
        for (int i = issues_size; i > 0; --i) {
            fprintf(
                stream, "%6d  %7.7s  %-s\n",
                issues[i - 1].number,
                issues[i - 1].state,
                issues[i - 1].title);
        }
    } else {
        for (int i = 0; i < issues_size; ++i) {
            fprintf(
                stream, "%6d  %7.7s  %-s\n",
                issues[i].number,
                issues[i].state,
                issues[i].title);
        }
    }
}

static void
ghcli_print_issue_summary(FILE *out, ghcli_issue_details *it)
{
    fprintf(out,
            "   NUMBER : %d\n"
            "    TITLE : "SV_FMT"\n"
            "  CREATED : "SV_FMT"\n"
            "   AUTHOR : "SV_FMT"\n"
            "    STATE : "SV_FMT"\n"
            " COMMENTS : %d\n"
            "   LOCKED : %s\n"
            "   LABELS : ",
            it->number,
            SV_ARGS(it->title), SV_ARGS(it->created_at),
            SV_ARGS(it->author), SV_ARGS(it->state),
            it->comments, sn_bool_yesno(it->locked));

    if (it->labels_size) {
        fprintf(out, SV_FMT, SV_ARGS(it->labels[0]));

        for (size_t i = 1; i < it->labels_size; ++i)
            fprintf(out, ", "SV_FMT, SV_ARGS(it->labels[0]));
    } else {
        fprintf(out, "none");
    }

    fputc('\n', out);

    if (it->assignees_size) {
        fprintf(out, "ASSIGNEES : "SV_FMT, SV_ARGS(it->assignees[0]));
        for (size_t i = 1; i < it->assignees_size; ++i)
            fprintf(out, ", "SV_FMT, SV_ARGS(it->assignees[i]));
    } else {
        fprintf(out, "ASSIGNEES : none\n");
    }

    fputc('\n', out);

    /* The API may not return a body if the user didn't put in any
     * comment */
    if (it->body.data) {
        pretty_print(it->body.data, 4, 80, out);
        fputc('\n', out);
    }
}

static void
ghcli_issue_details_free(ghcli_issue_details *it)
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
ghcli_issue_summary(
    FILE       *stream,
    const char *owner,
    const char *repo,
    int         issue_number)
{
    ghcli_issue_details  details = {0};

    ghcli_forge()->get_issue_summary(owner, repo, issue_number, &details);
    ghcli_print_issue_summary(stream, &details);
    ghcli_issue_details_free(&details);
}

void
ghcli_issue_close(const char *owner, const char *repo, int issue_number)
{
    ghcli_forge()->issue_close(owner, repo, issue_number);
}

void
ghcli_issue_reopen(const char *owner, const char *repo, int issue_number)
{
    ghcli_forge()->issue_reopen(owner, repo, issue_number);
}

static void
issue_init_user_file(FILE *stream, void *_opts)
{
    ghcli_submit_issue_options *opts = _opts;
    fprintf(
        stream,
        "# ISSUE TITLE : "SV_FMT"\n"
        "# Enter issue description below.\n"
        "# All lines starting with '#' will be discarded.\n",
        SV_ARGS(opts->title));
}

static sn_sv
ghcli_issue_get_user_message(ghcli_submit_issue_options *opts)
{
    return ghcli_editor_get_user_message(issue_init_user_file, opts);
}

void
ghcli_issue_submit(ghcli_submit_issue_options opts)
{
    ghcli_fetch_buffer  json_buffer  = {0};

    sn_sv body = ghcli_issue_get_user_message(&opts);

    opts.body = ghcli_json_escape(body);

    fprintf(stdout,
            "The following issue will be created:\n"
            "\n"
            "TITLE   : "SV_FMT"\n"
            "OWNER   : "SV_FMT"\n"
            "REPO    : "SV_FMT"\n"
            "MESSAGE :\n"SV_FMT"\n",
            SV_ARGS(opts.title), SV_ARGS(opts.owner),
            SV_ARGS(opts.repo), SV_ARGS(body));

    fputc('\n', stdout);

    if (!opts.always_yes) {
        if (!sn_yesno("Do you want to continue?"))
            errx(1, "Submission aborted.");
    }

    ghcli_forge()->perform_submit_issue(opts, &json_buffer);
    ghcli_print_html_url(json_buffer);

    free(body.data);
    free(opts.body.data);
    free(json_buffer.data);
}

void
ghcli_issue_assign(
    const char *owner,
    const char *repo,
    int         issue_number,
    const char *assignee)
{
    ghcli_forge()->issue_assign(owner, repo, issue_number, assignee);
}
