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

#ifndef GCLI_ISSUES_H
#define GCLI_ISSUES_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gcli/curl.h>
#include <gcli/issues.h>

int github_fetch_issues(gcli_ctx *ctx, char *url, int max,
                        gcli_issue_list *out);

int github_get_issues(gcli_ctx *ctx, char const *owner, char const *repo,
                      gcli_issue_fetch_details const *details, int max,
                      gcli_issue_list *out);

int github_get_issue_summary(gcli_ctx *ctx, char const *owner, char const *repo,
                             int issue_number, gcli_issue *out);

int github_issue_close(gcli_ctx *ctx, char const *owner, char const *repo,
                       int issue_number);

int github_issue_reopen(gcli_ctx *ctx, char const *owner, char const *repo,
                        int issue_number);

int github_perform_submit_issue(gcli_ctx *ctx, gcli_submit_issue_options opts,
                                gcli_fetch_buffer *out);

int github_issue_assign(gcli_ctx *ctx, char const *owner, char const *repo,
                        int issue_number, char const *assignee);

int github_issue_add_labels(gcli_ctx *ctx, char const *owner, char const *repo,
                            int issue, char const *const labels[],
                            size_t labels_size);

int github_issue_remove_labels(gcli_ctx *ctx, char const *owner,
                               char const *repo, int issue,
                               char const *const labels[], size_t labels_size);

int github_issue_set_milestone(gcli_ctx *ctx, char const *owner,
                               char const *repo, int issue, int milestone);

int github_issue_clear_milestone(gcli_ctx *ctx, char const *owner,
                                 char const *repo, int issue);

#endif /* GCLI_ISSUES_H */
