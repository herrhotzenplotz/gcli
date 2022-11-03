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

#ifndef GITHUB_PULLS_H
#define GITHUB_PULLS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gcli/curl.h>
#include <gcli/pulls.h>

int github_get_prs(char const *owner,
                   char const *reponame,
                   bool const all,
                   int const max,
                   gcli_pull **const out);

void github_print_pr_diff(FILE *stream,
                          char const *owner,
                          char const *reponame,
                          int const pr_number);

void github_pr_merge(char const *owner,
                     char const *reponame,
                     int const pr_number,
                     bool const squash);

void github_pr_reopen(char const *owner,
                      char const *reponame,
                      int const pr_number);

void github_pr_close(char const *owner,
                     char const *reponame,
                     int const pr_number);

void github_perform_submit_pr(gcli_submit_pull_options opts);

int github_get_pull_commits(char const *owner,
                            char const *repo,
                            int const pr_number,
                            gcli_commit **const out);

void github_get_pull_summary(char const *owner,
                             char const *repo,
                             int const pr_number,
                             gcli_pull_summary *const out);

sn_sv github_pull_try_derive_head(void);

#endif /* GITHUB_PULLS_H */
