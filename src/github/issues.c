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

#include <gcli/config.h>
#include <gcli/curl.h>
#include <gcli/github/config.h>
#include <gcli/github/issues.h>
#include <gcli/json_util.h>
#include <pdjson/pdjson.h>

#include <assert.h>

static void
github_parse_issue_entry(json_stream *input, gcli_issue *it)
{
	if (json_next(input) != JSON_OBJECT)
		errx(1, "Expected Issue Object");

	while (json_next(input) == JSON_STRING) {
		size_t      len = 0;
		const char *key = json_get_string(input, &len);

		if (strncmp("title", key, len) == 0)
			it->title = get_string(input);
		else if (strncmp("state", key, len) == 0)
			it->state = get_string(input);
		else if (strncmp("number", key, len) == 0)
			it->number = get_int(input);
		else if (strncmp("id", key, len) == 0)
			it->id = get_int(input);
		else
			SKIP_OBJECT_VALUE(input);
	}
}

int
github_get_issues(
	const char   *owner,
	const char   *repo,
	bool          all,
	int           max,
	gcli_issue **out)
{
	int                 count       = 0;
	json_stream         stream      = {0};
	gcli_fetch_buffer  json_buffer = {0};
	char               *url         = NULL;
	char               *e_owner     = NULL;
	char               *e_repo      = NULL;
	char               *next_url    = NULL;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/issues?state=%s",
		gcli_get_apibase(),
		e_owner, e_repo,
		all ? "all" : "open");

	do {
		gcli_fetch(url, &next_url, &json_buffer);

		json_open_buffer(&stream, json_buffer.data, json_buffer.length);
		json_set_streaming(&stream, true);

		enum json_type next_token = json_next(&stream);

		while ((next_token = json_peek(&stream)) != JSON_ARRAY_END) {
			switch (next_token) {
			case JSON_ERROR:
				errx(1, "Parser error: %s", json_get_error(&stream));
				break;
			case JSON_OBJECT: {
				*out = realloc(*out, sizeof(gcli_issue) * (count + 1));
				gcli_issue *it = &(*out)[count];
				memset(it, 0, sizeof(gcli_issue));
				github_parse_issue_entry(&stream, it);
				count += 1;
			} break;
			default:
				errx(1, "Unexpected json type in response");
				break;
			}

			if (count == max)
				break;
		}

		free(json_buffer.data);
		free(url);
		json_close(&stream);

	} while ((url = next_url) && (max == -1 || count < max));
	/* continue iterating if we have both a next_url and we are
	 * supposed to fetch more issues (either max is -1 thus all issues
	 * or we haven't fetched enough yet). */

	free(next_url);
	free(e_owner);
	free(e_repo);

	return count;
}

static void
github_parse_issue_details(json_stream *input, gcli_issue_details *out)
{
	enum json_type  key_type;
	const char     *key;

	json_next(input);

	while ((key_type = json_next(input)) == JSON_STRING) {
		size_t len;
		key = json_get_string(input, &len);

		if (strncmp("title", key, len) == 0)
			out->title = get_sv(input);
		else if (strncmp("state", key, len) == 0)
			out->state = get_sv(input);
		else if (strncmp("body", key, len) == 0)
			out->body = get_sv(input);
		else if (strncmp("created_at", key, len) == 0)
			out->created_at = get_sv(input);
		else if (strncmp("number", key, len) == 0)
			out->number = get_int(input);
		else if (strncmp("comments", key, len) == 0)
			out->comments = get_int(input);
		else if (strncmp("user", key, len) == 0)
			out->author = get_user_sv(input);
		else if (strncmp("locked", key, len) == 0)
			out->locked = get_bool(input);
		else if (strncmp("labels", key, len) == 0)
			out->labels_size = gcli_read_label_list(input, &out->labels);
		else if (strncmp("assignees", key, len) == 0)
			out->assignees_size = gcli_read_user_list(input, &out->assignees);
		else
			SKIP_OBJECT_VALUE(input);
	}

	if (key_type != JSON_OBJECT_END)
		errx(1, "Unexpected object key type");
}

void
github_get_issue_summary(
	const char          *owner,
	const char          *repo,
	int                  issue_number,
	gcli_issue_details *out)
{
	char               *url     = NULL;
	char               *e_owner = NULL;
	char               *e_repo  = NULL;
	gcli_fetch_buffer  buffer  = {0};
	json_stream         parser  = {0};

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

	github_parse_issue_details(&parser, out);

	json_close(&parser);
	free(url);
	free(e_owner);
	free(e_repo);
	free(buffer.data);
}

void
github_issue_close(const char *owner, const char *repo, int issue_number)
{
	gcli_fetch_buffer  json_buffer = {0};
	char               *url         = NULL;
	char               *data        = NULL;
	char               *e_owner     = NULL;
	char               *e_repo      = NULL;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/issues/%d",
		gcli_get_apibase(),
		e_owner, e_repo,
		issue_number);
	data = sn_asprintf("{ \"state\": \"close\"}");

	gcli_fetch_with_method("PATCH", url, data, NULL, &json_buffer);

	free(data);
	free(url);
	free(e_owner);
	free(e_repo);
	free(json_buffer.data);
}

void
github_issue_reopen(const char *owner, const char *repo, int issue_number)
{
	gcli_fetch_buffer  json_buffer = {0};
	char               *url         = NULL;
	char               *data        = NULL;
	char               *e_owner     = NULL;
	char               *e_repo      = NULL;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/issues/%d",
		gcli_get_apibase(),
		e_owner, e_repo,
		issue_number);
	data = sn_asprintf("{ \"state\": \"open\"}");

	gcli_fetch_with_method("PATCH", url, data, NULL, &json_buffer);

	free(data);
	free(url);
	free(e_owner);
	free(e_repo);
	free(json_buffer.data);
}

void
github_perform_submit_issue(
	gcli_submit_issue_options  opts,
	gcli_fetch_buffer         *out)
{
	sn_sv e_owner = gcli_urlencode_sv(opts.owner);
	sn_sv e_repo  = gcli_urlencode_sv(opts.repo);
	sn_sv e_title = gcli_json_escape(opts.title);
	sn_sv e_body  = gcli_json_escape(opts.body);

	char *post_fields = sn_asprintf(
		"{ \"title\": \""SV_FMT"\", \"body\": \""SV_FMT"\" }",
		SV_ARGS(e_title), SV_ARGS(e_body));
	char *url         = sn_asprintf(
		"%s/repos/"SV_FMT"/"SV_FMT"/issues",
		gcli_get_apibase(),
		SV_ARGS(e_owner),
		SV_ARGS(e_repo));

	gcli_fetch_with_method("POST", url, post_fields, NULL, out);

	free(e_owner.data);
	free(e_repo.data);
	free(e_title.data);
	free(e_body.data);
	free(post_fields);
	free(url);
}

void
github_issue_assign(
	const char *owner,
	const char *repo,
	int         issue_number,
	const char *assignee)
{
	gcli_fetch_buffer  buffer           = {0};
	sn_sv               escaped_assignee = SV_NULL;
	char               *post_fields      = NULL;
	char               *url              = NULL;
	char               *e_owner          = NULL;
	char               *e_repo           = NULL;

	escaped_assignee = gcli_json_escape(SV((char *)assignee));
	post_fields = sn_asprintf("{ \"assignees\": [\""SV_FMT"\"] }",
				  SV_ARGS(escaped_assignee));

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/issues/%d/assignees",
		gcli_get_apibase(), e_owner, e_repo, issue_number);

	gcli_fetch_with_method("POST", url, post_fields, NULL, &buffer);

	free(buffer.data);
	free(escaped_assignee.data);
	free(post_fields);
	free(e_owner);
	free(e_repo);
	free(url);
}

void
github_issue_add_labels(
	const char *owner,
	const char *repo,
	int         issue,
	const char *labels[],
	size_t      labels_size)
{
	char               *url    = NULL;
	char               *data   = NULL;
	char               *list   = NULL;
	gcli_fetch_buffer  buffer = {0};

	assert(labels_size > 0);

	url = sn_asprintf("%s/repos/%s/%s/issues/%d/labels",
			  gcli_get_apibase(), owner, repo, issue);

	list = sn_join_with(labels, labels_size, "\",\"");
	data = sn_asprintf("{ \"labels\": [\"%s\"]}", list);

	gcli_fetch_with_method("POST", url, data, NULL, &buffer);

	free(url);
	free(data);
	free(list);
	free(buffer.data);
}

void
github_issue_remove_labels(
	const char *owner,
	const char *repo,
	int         issue,
	const char *labels[],
	size_t      labels_size)
{
	char               *url     = NULL;
	char               *e_label = NULL;
	gcli_fetch_buffer  buffer  = {0};

	if (labels_size != 1)
		errx(1, "error: GitHub only supports removing labels from "
			 "issues one by one.");

	e_label = gcli_urlencode(labels[0]);

	url = sn_asprintf("%s/repos/%s/%s/issues/%d/labels/%s",
			  gcli_get_apibase(), owner, repo, issue, e_label);

	gcli_fetch_with_method("DELETE", url, NULL, NULL, &buffer);

	free(url);
	free(e_label);
	free(buffer.data);
}
