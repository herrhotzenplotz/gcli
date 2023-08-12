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

#include <gcli/config.h>
#include <gcli/curl.h>
#include <gcli/github/config.h>
#include <gcli/github/issues.h>
#include <gcli/json_util.h>
#include <pdjson/pdjson.h>

#include <assert.h>

#include <templates/github/issues.h>

/* TODO: Remove this function once we use linked lists for storing
 *       issues.
 *
 * This is an ugly hack caused by the sillyness of the Github API that
 * treats Pull Requests as issues and reports them to us when we
 * request issues. This function nukes them from the list, readjusts
 * the allocation size and fixes the reported list size. */
static void
github_hack_fixup_issues_that_are_actually_pulls(gcli_issue_list *const it)
{
	for (size_t i = it->issues_size; i > 0; --i) {
		if (it->issues[i-1].is_pr) {
			/*  len = 7, i = 5, to move = 7 - 5 = 2
			 *   0   1   2   3   4   5   6
			 * | x | x | x | x | X | x | x | */
			gcli_issue_free(&it->issues[i-1]);
			memmove(&it->issues[i-1], &it->issues[i],
			        sizeof(*it->issues) * (it->issues_size - i));
			it->issues = realloc(
				it->issues,
				(--it->issues_size) * sizeof(*it->issues));
		}
	}
}

int
github_fetch_issues(char *url,
                    int const max,
                    gcli_issue_list *const out)
{
	json_stream        stream      = {0};
	gcli_fetch_buffer  json_buffer = {0};
	char              *next_url    = NULL;

	do {
		gcli_fetch(url, &next_url, &json_buffer);

		json_open_buffer(&stream, json_buffer.data, json_buffer.length);

		parse_github_issues(&stream, &out->issues, &out->issues_size);
		/* Hack: Remove PRs from the issue list because github. */
		github_hack_fixup_issues_that_are_actually_pulls(out);

		free(json_buffer.data);
		free(url);
		json_close(&stream);

	} while ((url = next_url) && (max == -1 || (int)out->issues_size < max));

	free(url);

	return 0;
}

int
github_get_issues(char const *owner,
                  char const *repo,
                  gcli_issue_fetch_details const *details,
                  int const max,
                  gcli_issue_list *const out)
{
	char *url = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	char *e_author = NULL;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	if (details->author) {
		char *tmp = gcli_urlencode(details->author);
		e_author = sn_asprintf("&creator=%s", tmp);
		free(tmp);
	}

	url = sn_asprintf(
		"%s/repos/%s/%s/issues?state=%s%s",
		gcli_get_apibase(),
		e_owner, e_repo,
		details->all ? "all" : "open",
		e_author ? e_author : "");

	free(e_author);
	free(e_owner);
	free(e_repo);

	return github_fetch_issues(url, max, out);
}

void
github_get_issue_summary(char const *owner,
                         char const *repo,
                         int const issue_number,
                         gcli_issue *const out)
{
	char			  *url     = NULL;
	char			  *e_owner = NULL;
	char			  *e_repo  = NULL;
	gcli_fetch_buffer  buffer  = {0};
	json_stream		   parser  = {0};

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/issues/%d",
		gcli_get_apibase(),
		e_owner, e_repo,
		issue_number);
	gcli_fetch(url, NULL, &buffer);

	json_open_buffer(&parser, buffer.data, buffer.length);
	json_set_streaming(&parser, true);

	parse_github_issue(&parser, out);

	json_close(&parser);
	free(url);
	free(e_owner);
	free(e_repo);
	free(buffer.data);
}

int
github_issue_close(char const *owner, char const *repo, int const issue_number)
{
	char *url     = NULL;
	char *data    = NULL;
	char *e_owner = NULL;
	char *e_repo  = NULL;
	int   rc      = 0;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/issues/%d",
		gcli_get_apibase(),
		e_owner, e_repo,
		issue_number);
	data = sn_asprintf("{ \"state\": \"close\"}");

	rc = gcli_fetch_with_method("PATCH", url, data, NULL, NULL);

	free(data);
	free(url);
	free(e_owner);
	free(e_repo);

	return rc;
}

int
github_issue_reopen(char const *owner, char const *repo, int const issue_number)
{
	char *url     = NULL;
	char *data    = NULL;
	char *e_owner = NULL;
	char *e_repo  = NULL;
	int   rc      = 0;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/issues/%d",
		gcli_get_apibase(),
		e_owner, e_repo,
		issue_number);
	data = sn_asprintf("{ \"state\": \"open\"}");

	rc = gcli_fetch_with_method("PATCH", url, data, NULL, NULL);

	free(data);
	free(url);
	free(e_owner);
	free(e_repo);

	return rc;
}

int
github_perform_submit_issue(gcli_submit_issue_options opts,
                            gcli_fetch_buffer *out)
{
	char  *e_owner = gcli_urlencode(opts.owner);
	char  *e_repo  = gcli_urlencode(opts.repo);
	sn_sv  e_title = gcli_json_escape(opts.title);
	sn_sv  e_body  = gcli_json_escape(opts.body);
	int    rc      = 0;

	char *post_fields = sn_asprintf(
		"{ \"title\": \""SV_FMT"\", \"body\": \""SV_FMT"\" }",
		SV_ARGS(e_title), SV_ARGS(e_body));

	char *url = sn_asprintf("%s/repos/%s/%s/issues",
	                        gcli_get_apibase(), e_owner, e_repo);

	rc = gcli_fetch_with_method("POST", url, post_fields, NULL, out);

	free(e_owner);
	free(e_repo);
	free(e_title.data);
	free(e_body.data);
	free(post_fields);
	free(url);

	return rc;
}

int
github_issue_assign(char const *owner,
                    char const *repo,
                    int const issue_number,
                    char const *assignee)
{
	sn_sv  escaped_assignee = SV_NULL;
	char  *post_fields      = NULL;
	char  *url              = NULL;
	char  *e_owner          = NULL;
	char  *e_repo           = NULL;
	int    rc               = 0;

	escaped_assignee = gcli_json_escape(SV((char *)assignee));
	post_fields = sn_asprintf("{ \"assignees\": [\""SV_FMT"\"] }",
	                          SV_ARGS(escaped_assignee));

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/issues/%d/assignees",
		gcli_get_apibase(), e_owner, e_repo, issue_number);

	rc = gcli_fetch_with_method("POST", url, post_fields, NULL, NULL);

	free(escaped_assignee.data);
	free(post_fields);
	free(e_owner);
	free(e_repo);
	free(url);

	return rc;
}

int
github_issue_add_labels(char const *owner,
                        char const *repo,
                        int const issue,
                        char const *const labels[],
                        size_t const labels_size)
{
	char *url  = NULL;
	char *data = NULL;
	char *list = NULL;
	int   rc   = 0;

	assert(labels_size > 0);

	url = sn_asprintf("%s/repos/%s/%s/issues/%d/labels",
	                  gcli_get_apibase(), owner, repo, issue);

	list = sn_join_with(labels, labels_size, "\",\"");
	data = sn_asprintf("{ \"labels\": [\"%s\"]}", list);

	rc = gcli_fetch_with_method("POST", url, data, NULL, NULL);

	free(url);
	free(data);
	free(list);

	return rc;
}

int
github_issue_remove_labels(char const *owner,
                           char const *repo,
                           int const issue,
                           char const *const labels[],
                           size_t const labels_size)
{
	char *url     = NULL;
	char *e_label = NULL;
	int   rc      = 0;

	if (labels_size != 1)
		errx(1, "error: GitHub only supports removing labels from "
		     "issues one by one.");

	e_label = gcli_urlencode(labels[0]);

	url = sn_asprintf("%s/repos/%s/%s/issues/%d/labels/%s",
	                  gcli_get_apibase(), owner, repo, issue, e_label);

	rc = gcli_fetch_with_method("DELETE", url, NULL, NULL, NULL);

	free(url);
	free(e_label);

	return rc;
}

int
github_issue_set_milestone(char const *const owner,
                           char const *const repo,
                           int const issue,
                           int const milestone)
{
	char *url, *e_owner, *e_repo, *body;
	int rc;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/repos/%s/%s/issues/%d",
	                  gcli_get_apibase(),
	                  e_owner, e_repo, issue);

	body = sn_asprintf("{ \"milestone\": %d }", milestone);

	rc = gcli_fetch_with_method("PATCH", url, body, NULL, NULL);

	free(body);
	free(url);
	free(e_repo);
	free(e_owner);

	return rc;
}

int
github_issue_clear_milestone(char const *const owner,
                             char const *const repo,
                             int const issue)
{
	char *url, *e_owner, *e_repo, *body;
	int rc;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/repos/%s/%s/issues/%d",
	                  gcli_get_apibase(),
	                  e_owner, e_repo, issue);

	body = sn_asprintf("{ \"milestone\": null }");

	rc = gcli_fetch_with_method("PATCH", url, body, NULL, NULL);

	free(body);
	free(url);
	free(e_repo);
	free(e_owner);

	return rc;
}
