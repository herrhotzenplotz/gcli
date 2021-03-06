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
#include <gcli/gcli.h>

typedef struct gcli_pull                gcli_pull;
typedef struct gcli_submit_pull_options gcli_submit_pull_options;
typedef struct gcli_commit              gcli_commit;
typedef struct gcli_pull_summary        gcli_pull_summary;

struct gcli_pull {
	const char *title, *state, *creator;
	int number, id;
	bool merged;
};

struct gcli_pull_summary {
	char	*author;
	char    *state;
	char    *title;
	char    *body;
	char    *created_at;
	char    *commits_link;
	char	*head_label;
	char    *base_label;
	char    *head_sha;
	int      id;
	int      number;
	int      comments;
	int      additions;
	int      deletions;
	int      commits;
	int      changed_files;
	int      head_pipeline_id;	/* This is GitLab specific */
	sn_sv   *labels;
	size_t   labels_size;
	bool     merged;
	bool     mergeable;
	bool     draft;
};

struct gcli_commit {
	const char *sha, *message, *date, *author, *email;
};

/* Options to submit to the gh api for creating a PR */
struct gcli_submit_pull_options {
	sn_sv in;
	sn_sv from;
	sn_sv to;
	sn_sv title;
	sn_sv body;
	int   draft;
	bool  always_yes;
};

int gcli_get_prs(
	const char  *owner,
	const char  *reponame,
	bool         all,
	int          max,
	gcli_pull **out);

void gcli_pulls_free(
	gcli_pull *it,
	int         n);

void gcli_pulls_summary_free(
	gcli_pull_summary *it);

void gcli_print_pr_table(
	enum gcli_output_order  order,
	gcli_pull              *pulls,
	int                      pulls_size);

void gcli_print_pr_diff(
	FILE       *stream,
	const char *owner,
	const char *reponame,
	int         pr_number);

void gcli_pr_summary(
	const char *owner,
	const char *reponame,
	int         pr_number);

void gcli_pr_status(
	const char *owner,
	const char *reponame,
	int         pr_number);

void gcli_pr_submit(
	gcli_submit_pull_options);

void gcli_pr_merge(
	const char *owner,
	const char *reponame,
	int         pr_number,
	bool        squash);

void gcli_pr_close(
	const char *owner,
	const char *reponame,
	int         pr_number);

void gcli_pr_reopen(
	const char *owner,
	const char *reponame,
	int         pr_number);

void gcli_pr_add_labels(
	const char *owner,
	const char *repo,
	int         pr_number,
	const char *labels[],
	size_t      labels_size);

void gcli_pr_remove_labels(
	const char *owner,
	const char *repo,
	int         pr_number,
	const char *labels[],
	size_t      labels_size);

#endif /* PULLS_H */
