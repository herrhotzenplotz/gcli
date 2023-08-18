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

#ifndef GITLAB_MERGE_REQUESTS_H
#define GITLAB_MERGE_REQUESTS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gcli/pulls.h>

int gitlab_fetch_mrs(gcli_ctx *ctx, char *url, int max,
                     gcli_pull_list *list);

int gitlab_get_mrs(gcli_ctx *ctx, char const *owner,
                   char const *reponame,
                   gcli_pull_fetch_details const *details,
                   int max,
                   gcli_pull_list *out);

int gitlab_print_pr_diff(gcli_ctx *ctx, FILE *stream, char const *owner,
                         char const *reponame, int pr_number);

int gitlab_mr_merge(gcli_ctx *ctx, char const *owner, char const *reponame,
                    int mr_number, enum gcli_merge_flags flags);

int gitlab_mr_close(gcli_ctx *ctx, char const *owner, char const *reponame,
                    int pr_number);

int gitlab_mr_reopen(gcli_ctx *ctx, char const *owner, char const *reponame,
                     int pr_number);

int gitlab_get_pull(gcli_ctx *ctx, char const *owner, char const *repo,
                    int pr_number, gcli_pull *out);

int gitlab_get_pull_commits(gcli_ctx *ctx, char const *owner, char const *repo,
                            int pr_number, gcli_commit_list *out);

int gitlab_perform_submit_mr(gcli_ctx *ctx, gcli_submit_pull_options opts);

int gitlab_mr_add_labels(gcli_ctx *ctx, char const *owner, char const *repo,
                         int mr, char const *const labels[],
                         size_t labels_size);

int gitlab_mr_remove_labels(gcli_ctx *ctx, char const *owner, char const *repo,
                            int mr, char const *const labels[],
                            size_t labels_size);

int gitlab_mr_set_milestone(gcli_ctx *ctx, char const *owner, char const *repo,
                            int mr, int milestone_id);

int gitlab_mr_clear_milestone(gcli_ctx *ctx, char const *owner,
                              char const *repo, int mr);

#endif /* GITLAB_MERGE_REQUESTS_H */
