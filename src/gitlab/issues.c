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
#include <gcli/gitlab/config.h>
#include <gcli/gitlab/issues.h>
#include <gcli/json_util.h>

#include <templates/gitlab/issues.h>

#include <pdjson/pdjson.h>

/** Given the url fetch issues */
int
gitlab_fetch_issues(char *url,
                    int const max,
                    gcli_issue_list *const out)
{
	json_stream        stream      = {0};
	gcli_fetch_buffer  json_buffer = {0};
	char              *next_url    = NULL;

	do {
		gcli_fetch(url, &next_url, &json_buffer);

		json_open_buffer(&stream, json_buffer.data, json_buffer.length);

		parse_gitlab_issues(&stream, &out->issues, &out->issues_size);

		free(json_buffer.data);
		json_buffer.data = NULL;
		json_buffer.length = 0;

		free(url);
		json_close(&stream);
	} while ((url = next_url) && (max == -1 || (int)out->issues_size < max));
	/* continue iterating if we have both a next_url and we are
	 * supposed to fetch more issues (either max is -1 thus all issues
	 * or we haven't fetched enough yet). */

	free(next_url);

	return 0;
}

int
gitlab_get_issues(char const *owner,
                  char const *repo,
                  bool const all,
                  int const max,
                  gcli_issue_list *const out)
{
	char *url     = NULL;
	char *e_owner = NULL;
	char *e_repo  = NULL;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/projects/%s%%2F%s/issues%s",
		gitlab_get_apibase(),
		e_owner, e_repo,
		all ? "" : "?state=opened");

	free(e_owner);
	free(e_repo);

	return gitlab_fetch_issues(url, max, out);
}

void
gitlab_get_issue_summary(char const *owner,
                         char const *repo,
                         int const issue_number,
                         gcli_issue *const out)
{
	char              *url     = NULL;
	char              *e_owner = NULL;
	char              *e_repo  = NULL;
	gcli_fetch_buffer  buffer  = {0};
	json_stream        parser  = {0};

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/projects/%s%%2F%s/issues/%d",
		gitlab_get_apibase(),
		e_owner, e_repo,
		issue_number);
	gcli_fetch(url, NULL, &buffer);

	json_open_buffer(&parser, buffer.data, buffer.length);
	json_set_streaming(&parser, true);

	parse_gitlab_issue(&parser, out);

	json_close(&parser);
	free(url);
	free(e_owner);
	free(e_repo);
	free(buffer.data);
}


void
gitlab_issue_close(char const *owner, char const *repo, int const issue_number)
{
	gcli_fetch_buffer  json_buffer = {0};
	char              *url         = NULL;
	char              *data        = NULL;
	char              *e_owner     = NULL;
	char              *e_repo      = NULL;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url  = sn_asprintf(
		"%s/projects/%s%%2F%s/issues/%d",
		gitlab_get_apibase(),
		e_owner, e_repo,
		issue_number);
	data = sn_asprintf("{ \"state_event\": \"close\"}");

	gcli_fetch_with_method("PUT", url, data, NULL, &json_buffer);

	free(data);
	free(url);
	free(e_owner);
	free(e_repo);
	free(json_buffer.data);
}

void
gitlab_issue_reopen(char const *owner, char const *repo, int const issue_number)
{
	gcli_fetch_buffer  json_buffer = {0};
	char              *url         = NULL;
	char              *data        = NULL;
	char              *e_owner     = NULL;
	char              *e_repo      = NULL;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url  = sn_asprintf(
		"%s/projects/%s%%2F%s/issues/%d",
		gitlab_get_apibase(),
		e_owner, e_repo,
		issue_number);
	data = sn_asprintf("{ \"state_event\": \"reopen\"}");

	gcli_fetch_with_method("PUT", url, data, NULL, &json_buffer);

	free(data);
	free(url);
	free(e_owner);
	free(e_repo);
	free(json_buffer.data);
}

void
gitlab_perform_submit_issue(gcli_submit_issue_options opts,
                            gcli_fetch_buffer *const out)
{
	char *e_owner = gcli_urlencode(opts.owner);
	char *e_repo  = gcli_urlencode(opts.repo);
	sn_sv e_title = gcli_json_escape(opts.title);
	sn_sv e_body  = gcli_json_escape(opts.body);

	char *post_fields = sn_asprintf(
		"{ \"title\": \""SV_FMT"\", \"description\": \""SV_FMT"\" }",
		SV_ARGS(e_title), SV_ARGS(e_body));
	char *url         = sn_asprintf(
		"%s/projects/%s%%2F%s/issues",
		gitlab_get_apibase(), e_owner, e_repo);

	gcli_fetch_with_method("POST", url, post_fields, NULL, out);

	free(e_owner);
	free(e_repo);
	free(e_title.data);
	free(e_body.data);
	free(post_fields);
	free(url);
}

static int
gitlab_user_id(char const *user_name)
{
	gcli_fetch_buffer   buffer = {0};
	struct json_stream  stream = {0};
	char               *url    = NULL;
	char               *e_username;
	int                 uid    = -1;

	e_username = gcli_urlencode(user_name);

	url = sn_asprintf("%s/users?username=%s", gitlab_get_apibase(), e_username);

	gcli_fetch(url, NULL, &buffer);
	json_open_buffer(&stream, buffer.data, buffer.length);
	json_set_streaming(&stream, 1);

	gcli_json_advance(&stream, "[{s", "id");
	uid = get_int(&stream);

	json_close(&stream);

	free(e_username);
	free(url);
	free(buffer.data);

	return uid;
}

void
gitlab_issue_assign(char const *owner,
                    char const *repo,
                    int const issue_number,
                    char const *assignee)
{
	int                assignee_uid = -1;
	gcli_fetch_buffer  buffer       = {0};
	char              *url          = NULL;
	char              *post_data    = NULL;
	char              *e_owner      = NULL;
	char              *e_repo       = NULL;

	assignee_uid = gitlab_user_id(assignee);

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/issues/%d",
	                  gitlab_get_apibase(),
	                  e_owner, e_repo, issue_number);
	post_data = sn_asprintf("{ \"assignee_ids\": [ %d ] }", assignee_uid);
	gcli_fetch_with_method("PUT", url, post_data, NULL, &buffer);

	free(e_owner);
	free(e_repo);
	free(buffer.data);
	free(url);
	free(post_data);
}

void
gitlab_issue_add_labels(char const *owner,
                        char const *repo,
                        int const issue,
                        char const *const labels[],
                        size_t const labels_size)
{
	char              *url    = NULL;
	char              *data   = NULL;
	char              *list   = NULL;
	gcli_fetch_buffer  buffer = {0};

	url = sn_asprintf("%s/projects/%s%%2F%s/issues/%d",
	                  gitlab_get_apibase(), owner, repo, issue);

	list = sn_join_with(labels, labels_size, ",");
	data = sn_asprintf("{ \"add_labels\": \"%s\"}", list);

	gcli_fetch_with_method("PUT", url, data, NULL, &buffer);

	free(url);
	free(data);
	free(list);
	free(buffer.data);
}

void
gitlab_issue_remove_labels(char const *owner,
                           char const *repo,
                           int const issue,
                           char const *const labels[],
                           size_t const labels_size)
{
	char              *url    = NULL;
	char              *data   = NULL;
	char              *list   = NULL;
	gcli_fetch_buffer  buffer = {0};

	url = sn_asprintf("%s/projects/%s%%2F%s/issues/%d",
	                  gitlab_get_apibase(), owner, repo, issue);

	list = sn_join_with(labels, labels_size, ",");
	data = sn_asprintf("{ \"remove_labels\": \"%s\"}", list);

	gcli_fetch_with_method("PUT", url, data, NULL, &buffer);

	free(url);
	free(data);
	free(list);
	free(buffer.data);
}

int
gitlab_issue_set_milestone(char const *const owner,
                           char const *const repo,
                           int const issue,
                           int const milestone)
{
	char *url, *e_owner, *e_repo;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);
	url = sn_asprintf("%s/projects/%s%%2F%s/issues/%d?milestone_id=%d",
	                  gitlab_get_apibase(), e_owner, e_repo, issue, milestone);

	gcli_fetch_with_method("PUT", url, NULL, NULL, NULL);

	free(url);
	free(e_repo);
	free(e_owner);

	return 0;
}
