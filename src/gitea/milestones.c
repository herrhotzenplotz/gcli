/*
 * Copyright 2023 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/gitea/milestones.h>

#include <gcli/curl.h>
#include <gcli/config.h>

#include <gcli/github/issues.h>
#include <gcli/github/milestones.h>

#include <templates/gitea/milestones.h>

#include <pdjson/pdjson.h>

int
gitea_get_milestones(char const *const owner,
                     char const *const repo,
                     int const max,
                     gcli_milestone_list *const out)
{
	char *url = NULL, *next_url = NULL;
	char *e_owner, *e_repo;
	gcli_fetch_buffer buffer = {0};
	json_stream stream = {0};

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/repos/%s/%s/milestones",
	                  gcli_get_apibase(), e_owner, e_repo);

	do {
		gcli_fetch(url, &next_url, &buffer);
		json_open_buffer(&stream, buffer.data, buffer.length);

		parse_gitea_milestones(&stream, &out->milestones, &out->milestones_size);

		json_close(&stream);
		free(url);
		free(buffer.data);
	} while ((url = next_url) && ((max < 0) || (out->milestones_size < (size_t)max)));

	free(url);
	free(e_owner);
	free(e_repo);

	return 0;
}

int
gitea_get_milestone(char const *const owner,
                    char const *const repo,
                    int const milestone,
                    gcli_milestone *const out)
{
	char *url, *e_owner, *e_repo;
	gcli_fetch_buffer buffer = {0};
	json_stream stream = {0};

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/repos/%s/%s/milestones/%d",
	                  gcli_get_apibase(), e_owner, e_repo, milestone);

	gcli_fetch(url, NULL, &buffer);
	json_open_buffer(&stream, buffer.data, buffer.length);

	parse_gitea_milestone(&stream, out);

	json_close(&stream);

	free(buffer.data);
	free(url);

	free(e_owner);
	free(e_repo);

	return 0;
}

int
gitea_create_milestone(struct gcli_milestone_create_args const *args)
{
	return github_create_milestone(args);
}

int
gitea_milestone_get_issues(char const *const owner,
                           char const *const repo,
                           int const milestone,
                           gcli_issue_list *const out)
{
	char *url, *e_owner, *e_repo;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/repos/%s/%s/issues?state=all&milestones=%d",
	                  gcli_get_apibase(), e_owner, e_repo, milestone);

	free(e_repo);
	free(e_owner);

	return github_fetch_issues(url, -1, out);
}

int
gitea_delete_milestone(char const *const owner,
                       char const *const repo,
                       int const milestone)
{
	return github_delete_milestone(owner, repo, milestone);
}

int
gitea_milestone_set_duedate(char const *const owner,
                            char const *const repo,
                            int const milestone,
                            char const *const date)
{
	return github_milestone_set_duedate(owner, repo, milestone, date);
}
