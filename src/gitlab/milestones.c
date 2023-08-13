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

#include <gcli/curl.h>
#include <gcli/gitlab/config.h>
#include <gcli/gitlab/issues.h>
#include <gcli/gitlab/merge_requests.h>
#include <gcli/gitlab/milestones.h>
#include <gcli/json_util.h>

#include <templates/gitlab/milestones.h>

#include <pdjson/pdjson.h>

#include <assert.h>
#include <time.h>

int
gitlab_get_milestones(char const *owner,
                      char const *repo,
                      int max,
                      gcli_milestone_list *const out)
{
	char *url, *next_url = NULL;
	char *e_owner, *e_repo;
	gcli_fetch_buffer buffer = {0};
	json_stream stream = {0};

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/milestones",
	                  gitlab_get_apibase(), e_owner, e_repo);

	do {
		gcli_fetch(url, &next_url, &buffer);
		json_open_buffer(&stream, buffer.data, buffer.length);

		parse_gitlab_milestones(&stream, &out->milestones, &out->milestones_size);

		free(buffer.data);
		free(url);
		json_close(&stream);
	} while ((url = next_url) && (max == -1 || (int)out->milestones_size < max));

	free(next_url);

	free(e_owner);
	free(e_repo);

	return 0;
}

int
gitlab_get_milestone(char const *owner,
                     char const *repo,
                     int const milestone,
                     gcli_milestone *const out)
{
	char *url, *e_owner, *e_repo;
	gcli_fetch_buffer buffer = {0};
	json_stream stream = {0};
	int rc = 0;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/milestones/%d",
	                  gitlab_get_apibase(), e_owner, e_repo, milestone);

	gcli_fetch(url, NULL, &buffer);
	json_open_buffer(&stream, buffer.data, buffer.length);

	parse_gitlab_milestone(&stream, out);

	json_close(&stream);
	free(buffer.data);
	free(url);
	free(e_owner);
	free(e_repo);

	return rc;
}

int
gitlab_milestone_get_issues(char const *const owner,
                            char const *const repo,
                            int const milestone,
                            gcli_issue_list *const out)
{
	char *url, *e_owner, *e_repo;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/milestones/%d/issues",
	                  gitlab_get_apibase(), e_owner, e_repo, milestone);

	free(e_repo);
	free(e_owner);
	/* URL is freed by the fetch_issues call */

	return gitlab_fetch_issues(url, -1, out);;
}

int
gitlab_create_milestone(struct gcli_milestone_create_args const *args)
{
	char *url, *e_owner, *e_repo, *e_title, *json_body, *description = NULL;
	int rc = 0;

	e_owner = gcli_urlencode(args->owner);
	e_repo = gcli_urlencode(args->repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/milestones",
	                  gitlab_get_apibase(), e_owner, e_repo);

	/* Escape and prepare the description if needed */
	if (args->description) {
		char *e_description = gcli_json_escape_cstr(args->description);
		description = sn_asprintf(", \"description\": \"%s\"", e_description);
		free(e_description);
	}

	e_title = gcli_json_escape_cstr(args->title);

	json_body = sn_asprintf("{"
	                        "    \"title\": \"%s\""
	                        "    %s"
	                        "}",
	                        e_title, description ? description : "");

	rc = gcli_fetch_with_method("POST", url, json_body, NULL, NULL);

	free(json_body);
	free(description);
	free(url);
	free(e_title);
	free(e_repo);
	free(e_owner);

	return rc;
}

int
gitlab_delete_milestone(char const *const owner,
                        char const *const repo,
                        int const milestone)
{
	char *url, *e_owner, *e_repo;
	int rc = 0;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/milestones/%d",
	                  gitlab_get_apibase(), e_owner, e_repo, milestone);

	rc = gcli_fetch_with_method("DELETE", url, NULL, NULL, NULL);

	free(url);
	free(e_repo);
	free(e_owner);

	return rc;
}

/* TODO: merge this with the github code */
static void
normalize_date_to_gitlab_format(char const *const input, char *output,
                                size_t const output_size)
{
	struct tm tm_buf = {0};
	struct tm *utm_buf;
	char *endptr;
	time_t utctime;

	assert(output_size == 9);

	/* Parse input time */
	endptr = strptime(input, "%Y-%m-%d", &tm_buf);
	if (endptr == NULL || *endptr != '\0')
		errx(1, "error: date »%s« is invalid: want YYYY-MM-DD", input);

	/* Convert to UTC: Really, we should be using the _r versions of
	 * these functions for thread-safety but since gcli doesn't do
	 * multithreading (except for inside libcurl) we do not need to be
	 * worried about the storage behind the pointer returned by gmtime
	 * to be altered by another thread. */
	utctime = mktime(&tm_buf);
	utm_buf = gmtime(&utctime);

	/* Format the output string - now in UTC */
	strftime(output, output_size, "%Y%m%d", utm_buf);
}

int
gitlab_milestone_set_duedate(char const *const owner,
                             char const *const repo,
                             int const milestone,
                             char const *const date)
{
	char *url, *e_owner, *e_repo, norm_date[9] = {0};
	int rc = 0;

	normalize_date_to_gitlab_format(date, norm_date, sizeof norm_date);

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/milestones/%d?due_date=%s",
	                  gitlab_get_apibase(), e_owner, e_repo, milestone,
	                  norm_date);

	rc = gcli_fetch_with_method("PUT", url, "", NULL, NULL);

	free(url);
	free(e_repo);
	free(e_owner);

	return rc;
}
