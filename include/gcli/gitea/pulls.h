/*
 * Copyright 2022 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#ifndef GITEA_PULLS_H
#define GITEA_PULLS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gcli/curl.h>
#include <gcli/pulls.h>

int gitea_get_pulls(char const *owner,
                    char const *reponame,
                    gcli_pull_fetch_details const *details,
                    int max,
                    gcli_pull_list *out);

int gitea_get_pull(char const *owner,
                   char const *repo,
                   int pr_number,
                   gcli_pull *out);

int gitea_get_pull_commits(char const *owner,
                           char const *repo,
                           int pr_number,
                           gcli_commit **out);

int gitea_pull_submit(gcli_submit_pull_options opts);

int gitea_pull_merge(char const *owner,
                     char const *reponame,
                     int pr_number,
                     enum gcli_merge_flags flags);

int gitea_pull_close(char const *owner,
                     char const *repo,
                     int pr_number);

int gitea_pull_reopen(char const *owner,
                      char const *repo,
                      int pr_number);

void gitea_print_pr_diff(FILE *stream,
                         char const *owner,
                         char const *repo,
                         int pr_number);

int gitea_pull_checks(char const *owner,
                      char const *repo,
                      int pr_number);

int gitea_pull_set_milestone(char const *owner,
                             char const *repo,
                             int pr_number,
                             int milestone_id);

int gitea_pull_clear_milestone(char const *owner,
                               char const *repo,
                               int pr_number);

#endif /* GITEA_PULLS_H */
