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

#ifndef PULLS_H
#define PULLS_H

#include <stdio.h>
#include <stdbool.h>

#include <sn/sn.h>
#include <ghcli/ghcli.h>

typedef struct ghcli_pull                ghcli_pull;
typedef struct ghcli_submit_pull_options ghcli_submit_pull_options;
typedef struct ghcli_commit              ghcli_commit;
typedef struct ghcli_pull_summary        ghcli_pull_summary;

struct ghcli_pull {
    const char *title, *state, *creator;
    int number, id;
    bool merged;
};

struct ghcli_pull_summary {
    const char *author, *state, *title, *body, *created_at, *commits_link;
    int         id, number, comments, additions, deletions, commits, changed_files;
    sn_sv      *labels;
    size_t      labels_size;
    bool        merged, mergeable, draft;
};

struct ghcli_commit {
    const char *sha, *message, *date, *author, *email;
};

/* Options to submit to the gh api for creating a PR */
struct ghcli_submit_pull_options {
    sn_sv in;
    sn_sv from;
    sn_sv to;
    sn_sv title;
    sn_sv body;
    int   draft;
    bool  always_yes;
};

int ghcli_get_prs(
    const char  *owner,
    const char  *reponame,
    bool         all,
    int          max,
    ghcli_pull **out);
void ghcli_pulls_free(
    ghcli_pull *it,
    int         n);
void ghcli_pulls_summary_free(
    ghcli_pull_summary *it);
void ghcli_print_pr_table(
    FILE                    *stream,
    enum ghcli_output_order  order,
    ghcli_pull              *pulls,
    int                      pulls_size);
void ghcli_print_pr_diff(
    FILE       *stream,
    const char *owner,
    const char *reponame,
    int         pr_number);
void ghcli_pr_summary(
    FILE       *stream,
    const char *owner,
    const char *reponame,
    int         pr_number);
void ghcli_pr_submit(
    ghcli_submit_pull_options);
void ghcli_pr_merge(
    FILE       *stream,
    const char *owner,
    const char *reponame,
    int         pr_number);
void ghcli_pr_close(
    const char *owner,
    const char *reponame,
    int         pr_number);
void ghcli_pr_reopen(
    const char *owner,
    const char *reponame,
    int         pr_number);

#endif /* PULLS_H */
