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

#ifndef GITEA_ISSUES_H
#define GITEA_ISSUES_H

#include <gcli/issues.h>

int gitea_get_issues(
	const char   *owner,
	const char   *reponame,
	bool          all,
	int           max,
	gcli_issue	**out);
void gitea_get_issue_summary(
	const char	*owner,
	const char	*repo,
	int			 issue,
	gcli_issue	*out);
void gitea_submit_issue(
	gcli_submit_issue_options	 opts,
	gcli_fetch_buffer			*out);
void gitea_issue_close(
	const char	*owner,
	const char	*repo,
	int			 issue_number);
void gitea_issue_reopen(
	const char	*owner,
	const char	*repo,
	int			 issue_number);
void gitea_issue_assign(
	const char	*owner,
	const char	*repo,
	int			 issue_number,
	const char	*assignee);
void gitea_issue_add_labels(
	const char	*owner,
	const char	*repo,
	int			 issue,
	const char	*labels[],
	size_t		 labels_size);
void gitea_issue_remove_labels(
	const char	*owner,
	const char	*repo,
	int			 issue,
	const char	*labels[],
	size_t		 labels_size);

#endif /* GITEA_ISSUES_H */
