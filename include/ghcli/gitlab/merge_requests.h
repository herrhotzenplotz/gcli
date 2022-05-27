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

#ifndef GITLAB_MERGE_REQUESTS_H
#define GITLAB_MERGE_REQUESTS_H

#include <ghcli/pulls.h>

int gitlab_get_mrs(
	const char  *owner,
	const char  *reponame,
	bool         all,
	int          max,
	ghcli_pull **out);

void gitlab_print_pr_diff(
	FILE       *stream,
	const char *owner,
	const char *reponame,
	int         pr_number);

void gitlab_mr_merge(
	const char *owner,
	const char *reponame,
	int         mr_number,
	bool        squash);

void gitlab_mr_close(
	const char *owner,
	const char *reponame,
	int         pr_number);

void gitlab_mr_reopen(
	const char *owner,
	const char *reponame,
	int         pr_number);

void gitlab_get_pull_summary(
	const char         *owner,
	const char         *repo,
	int                 pr_number,
	ghcli_pull_summary *out);

int gitlab_get_pull_commits(
	const char    *owner,
	const char    *repo,
	int            pr_number,
	ghcli_commit **out);

void gitlab_perform_submit_mr(
	ghcli_submit_pull_options  opts,
	ghcli_fetch_buffer        *out);

void gitlab_mr_add_labels(
	const char *owner,
	const char *repo,
	int         mr,
	const char *labels[],
	size_t      labels_size);

void gitlab_mr_remove_labels(
	const char *owner,
	const char *repo,
	int         mr,
	const char *labels[],
	size_t      labels_size);

#endif /* GITLAB_MERGE_REQUESTS_H */
