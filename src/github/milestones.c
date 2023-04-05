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

#include <gcli/github/milestones.h>

#include <gcli/config.h>
#include <gcli/curl.h>
#include <gcli/github/issues.h>
#include <gcli/json_util.h>

#include <templates/github/milestones.h>

#include <pdjson/pdjson.h>

#include <assert.h>
#include <time.h>

int
github_get_milestones(char const *const owner,
                      char const *const repo,
                      int const max,
                      gcli_milestone_list *const out)
{
	char *url, *next_url = NULL, *e_owner, *e_repo;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/repos/%s/%s/milestones",
	                  gcli_get_apibase(),
	                  e_owner, e_repo);

	do {
		gcli_fetch_buffer buffer = {0};
		json_stream stream = {0};

		gcli_fetch(url, &next_url, &buffer);
		json_open_buffer(&stream, buffer.data, buffer.length);

		parse_github_milestones(&stream, &out->milestones, &out->milestones_size);

		json_close(&stream);
		free(url);
		free(buffer.data);
	} while ((url = next_url) && ((max < 0) || (out->milestones_size < (size_t)max)));

	free(url);

	return 0;
}

int
github_get_milestone(char const *const owner,
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

	parse_github_milestone(&stream, out);

	json_close(&stream);
	free(url);
	free(buffer.data);
	free(e_repo);
	free(e_owner);

	return 0;
}

int
github_milestone_get_issues(char const *const owner,
                            char const *const repo,
                            int const milestone,
                            gcli_issue_list *const out)
{
	char *url, *e_owner, *e_repo;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/repos/%s/%s/issues?milestone=%d&state=all",
	                  gcli_get_apibase(), e_owner, e_repo, milestone);

	free(e_repo);
	free(e_owner);
	/* URL is freed by github_fetch_issues */

	return github_fetch_issues(url, -1, out);
}

int
github_create_milestone(struct gcli_milestone_create_args const *args)
{
	char *url, *e_owner, *e_repo;
	char *json_body, *description;

	e_owner = gcli_urlencode(args->owner);
	e_repo = gcli_urlencode(args->repo);

	if (args->description) {
		/* This is fine :-) */
		char *e_description = gcli_json_escape_cstr(args->description);
		description = sn_asprintf(",\"description\": \"%s\"", e_description);
		free(e_description);
	} else {
		description = strdup("");
	}

	json_body = sn_asprintf(
		"{"
		"    \"title\"      : \"%s\""
		"    %s"
		"}", args->title, description);

	url = sn_asprintf("%s/repos/%s/%s/milestones",
	                  gcli_get_apibase(), e_owner, e_repo);

	gcli_fetch_with_method("POST", url, json_body, NULL, NULL);

	free(json_body);
	free(description);
	free(url);
	free(e_repo);
	free(e_owner);

	return 0;
}

int
github_delete_milestone(char const *const owner,
                        char const *const repo,
                        int const milestone)
{
	char *url, *e_owner, *e_repo;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/repos/%s/%s/milestones/%d",
	                  gcli_get_apibase(),
	                  e_owner, e_repo,
	                  milestone);

	gcli_fetch_with_method("DELETE", url, NULL, NULL, NULL);

	free(url);
	free(e_repo);
	free(e_owner);

	return 0;
}

static void
normalize_date_to_iso8601(char const *const input,
                          char *output, size_t const output_size)
{
	struct tm tm_buf = {0};
	struct tm *utm_buf;
	char *endptr;
	time_t utctime;

	assert(output_size == 21);

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
	strftime(output, output_size, "%Y-%m-%dT%H:%M:%SZ", utm_buf);
}

int
github_milestone_set_duedate(char const *const owner,
                             char const *const repo,
                             int const milestone,
                             char const *const date)
{
	char *url, *e_owner, *e_repo, *payload, norm_date[21] = {0};

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/repos/%s/%s/milestones/%d",
	                  gcli_get_apibase(),
	                  e_owner, e_repo, milestone);

	normalize_date_to_iso8601(date, norm_date, sizeof norm_date);

	payload = sn_asprintf("{ \"due_on\": \"%s\"}", norm_date);
	gcli_fetch_with_method("PATCH", url, payload, NULL, NULL);

	free(payload);
	free(url);
	free(e_repo);
	free(e_owner);

	return 0;
}
