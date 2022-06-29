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

#include <ghcli/gitea/pulls.h>
#include <ghcli/github/pulls.h>
#include <ghcli/json_util.h>

int
gitea_get_pulls(
	const char  *owner,
	const char  *repo,
	bool         all,
	int          max,
	ghcli_pull **out)
{
	return github_get_prs(owner, repo, all, max, out);
}

void
gitea_get_pull_summary(
	const char			*owner,
	const char			*repo,
	int					 pr_number,
	ghcli_pull_summary	*out)
{
	github_get_pull_summary(owner, repo, pr_number, out);
}

int
gitea_get_pull_commits(
	const char    *owner,
	const char    *repo,
	int            pr_number,
	ghcli_commit **out)
{
	return github_get_pull_commits(owner, repo, pr_number, out);
}

void
gitea_pull_submit(
	ghcli_submit_pull_options  opts,
	ghcli_fetch_buffer        *out)
{
	warnx("In case the following process errors out, see: "
		  "https://github.com/go-gitea/gitea/issues/20175");
	github_perform_submit_pr(opts, out);
}
