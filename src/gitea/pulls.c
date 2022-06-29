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

#include <ghcli/config.h>
#include <ghcli/gitea/pulls.h>
#include <ghcli/github/pulls.h>

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

void
gitea_pull_merge(
	const char	*owner,
	const char	*repo,
	int			 pr_number,
	bool		 squash)
{
	char				*url	 = NULL;
	char				*e_owner = NULL;
	char				*e_repo	 = NULL;
	char				*data	 = NULL;
	ghcli_fetch_buffer	 buffer	 = {0};

	e_owner = ghcli_urlencode(owner);
	e_repo	= ghcli_urlencode(repo);
	url		= sn_asprintf("%s/repos/%s/%s/pulls/%d/merge",
						  ghcli_get_apibase(), e_owner, e_repo, pr_number);
	data	= sn_asprintf("{ \"Do\": \"%s\" }", squash ? "squash" : "merge");

	ghcli_fetch_with_method("POST", url, data, NULL, &buffer);

	free(url);
	free(e_owner);
	free(e_repo);
	free(data);
	free(buffer.data);
}

static void
gitea_pulls_patch_state(
	const char	*owner,
	const char	*repo,
	int			 pr_number,
	const char	*state)
{
	ghcli_fetch_buffer  json_buffer = {0};
	char               *url         = NULL;
	char               *data        = NULL;
	char               *e_owner     = NULL;
	char               *e_repo      = NULL;

	e_owner = ghcli_urlencode(owner);
	e_repo  = ghcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/pulls/%d",
		ghcli_get_apibase(),
		e_owner, e_repo,
		pr_number);
	data = sn_asprintf("{ \"state\": \"%s\"}", state);

	ghcli_fetch_with_method("PATCH", url, data, NULL, &json_buffer);

	free(data);
	free(url);
	free(e_owner);
	free(e_repo);
	free(json_buffer.data);
}

void
gitea_pull_close(
	const char	*owner,
	const char	*repo,
	int			 pr_number)
{
	gitea_pulls_patch_state(owner, repo, pr_number, "closed");
}

void
gitea_pull_reopen(
	const char	*owner,
	const char	*repo,
	int			 pr_number)
{
	gitea_pulls_patch_state(owner, repo, pr_number, "open");
}

void
gitea_print_pr_diff(
	FILE       *stream,
	const char *owner,
	const char *repo,
	int         pr_number)
{
	char *url     = NULL;
	char *e_owner = NULL;
	char *e_repo  = NULL;

	e_owner = ghcli_urlencode(owner);
	e_repo  = ghcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/pulls/%d.patch",
		ghcli_get_apibase(),
		e_owner, e_repo, pr_number);
	ghcli_curl(stream, url, NULL);

	free(e_owner);
	free(e_repo);
	free(url);
}
