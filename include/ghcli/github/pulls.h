/*
 * Copyright 2021 Nico Sonack <nsonack@outlook.com>
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

#ifndef GITHUB_PULLS_H
#define GITHUB_PULLS_H

#include <ghcli/curl.h>
#include <ghcli/pulls.h>

int github_get_prs(
    const char  *owner,
    const char  *reponame,
    bool         all,
    int          max,
    ghcli_pull **out);

void github_print_pr_diff(
    FILE       *stream,
    const char *owner,
    const char *reponame,
    int         pr_number);

void github_pr_merge(
    FILE       *out,
    const char *owner,
    const char *reponame,
    int         pr_number);

void github_pr_reopen(
    const char *owner,
    const char *reponame,
    int         pr_number);

void github_pr_close(
    const char *owner,
    const char *reponame,
    int         pr_number);

void github_perform_submit_pr(
    ghcli_submit_pull_options  opts,
    ghcli_fetch_buffer        *out);

int github_get_pull_commits(
    const char    *owner,
    const char    *repo,
    int            pr_number,
    ghcli_commit **out);

void github_get_pull_summary(
    const char         *owner,
    const char         *repo,
    int                 pr_number,
    ghcli_pull_summary *out);

sn_sv github_pull_try_derive_head(void);

#endif /* GITHUB_PULLS_H */
