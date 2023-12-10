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

#ifndef ISSUES_H
#define ISSUES_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sn/sn.h>
#include <gcli/gcli.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct gcli_issue gcli_issue;
typedef struct gcli_submit_issue_options gcli_submit_issue_options;
typedef struct gcli_issue_list gcli_issue_list;
typedef struct gcli_issue_fetch_details gcli_issue_fetch_details;

struct gcli_issue {
	gcli_id number;
	char *title;
	char *product;     /* only on Bugzilla */
	char *component;   /* only on Bugzilla */
	char *created_at;
	char *author;
	char *state;
	int comments;
	bool locked;
	char *body;
	char **labels;
	size_t labels_size;
	char **assignees;
	size_t assignees_size;
	/* workaround for GitHub where PRs are also issues */
	int is_pr;
	char *milestone;
};

struct gcli_submit_issue_options {
	char const *owner;
	char const *repo;
	char *title;
	char *body;
};

struct gcli_issue_list {
	gcli_issue *issues;
	size_t issues_size;
};

struct gcli_issue_fetch_details {
	bool all;                   /* disregard the issue state */
	char const *author;         /* filter issues by this author*/
	char const *label;          /* filter by the given label */
	char const *milestone;      /* filter by the given milestone */
};

int gcli_get_issues(gcli_ctx *ctx, char const *owner, char const *reponame,
                    gcli_issue_fetch_details const *details, int max,
                    gcli_issue_list *out);

void gcli_issues_free(gcli_issue_list *);

int gcli_get_issue(gcli_ctx *ctx, char const *owner, char const *reponame,
                   gcli_id issue_number, gcli_issue *out);

void gcli_issue_free(gcli_issue *it);

int gcli_issue_close(gcli_ctx *ctx, char const *owner, char const *repo,
                     gcli_id issue_number);

int gcli_issue_reopen(gcli_ctx *ctx, char const *owner, char const *repo,
                      gcli_id issue_number);

int gcli_issue_submit(gcli_ctx *ctx, gcli_submit_issue_options);

int gcli_issue_assign(gcli_ctx *ctx, char const *owner, char const *repo,
                      gcli_id issue_number, char const *assignee);

int gcli_issue_add_labels(gcli_ctx *ctx, char const *owner, char const *repo,
                          gcli_id issue_number, char const *const labels[],
                          size_t labels_size);

int gcli_issue_remove_labels(gcli_ctx *ctx, char const *owner, char const *repo,
                             gcli_id issue_number, char const *const labels[],
                             size_t labels_size);

int gcli_issue_set_milestone(gcli_ctx *ctx, char const *owner, char const *repo,
                             gcli_id issue, int milestone);

int gcli_issue_clear_milestone(gcli_ctx *cxt, char const *owner,
                               char const *repo, gcli_id issue);

int gcli_issue_set_title(gcli_ctx *ctx, char const *owner, char const *repo,
                         gcli_id issue, char const *new_title);

#endif /* ISSUES_H */
