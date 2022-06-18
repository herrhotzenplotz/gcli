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

#include <ghcli/curl.h>
#include <ghcli/gitea/config.h>
#include <ghcli/gitea/issues.h>
#include <ghcli/json_util.h>

#include <pdjson/pdjson.h>

static void
gitea_parse_issue(struct json_stream *input, ghcli_issue *it)
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
gitea_get_issues(
	const char   *owner,
	const char   *repo,
	bool          all,
	int           max,
	ghcli_issue **out)
{
	char				*url	  = NULL;
	char				*next_url = NULL;
	char				*e_owner  = NULL;
	char				*e_repo	  = NULL;
	ghcli_fetch_buffer	 buffer	  = {0};
	struct json_stream	 stream	  = {0};
	enum json_type		 next	  = JSON_NULL;
	int					 out_size = 0;

	*out = NULL;

	e_owner = ghcli_urlencode(owner);
	e_repo  = ghcli_urlencode(repo);

	url = sn_asprintf("%s/repos/%s/%s/issues%s",
					  gitea_get_apibase(),
					  owner, repo,
					  all ? "?state=all" : "");

	do {
		ghcli_fetch(url, &next_url, &buffer);
		json_open_buffer(&stream, buffer.data, buffer.length);

		next = json_next(&stream);
		if (next != JSON_ARRAY)
			errx(1, "error: expected a json array in api response");

		while (json_peek(&stream) == JSON_OBJECT) {
			*out = realloc(*out, sizeof(**out) * (out_size + 1));

			ghcli_issue *it = &(*out)[out_size++];
			gitea_parse_issue(&stream, it);

			if (out_size == max)
				break;
		}

		json_close(&stream);
		free(url);
		free(buffer.data);
	} while ((url = next_url) && (max == -1 || out_size < max));

	free(e_owner);
	free(e_repo);

	return out_size;
}

static void
gitea_parse_issue_details(struct json_stream *input, ghcli_issue_details *out)
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
			out->labels_size = ghcli_read_label_list(input, &out->labels);
		else if (strncmp("assignees", key, len) == 0)
			out->assignees_size = ghcli_read_user_list(input, &out->assignees);
		else
			SKIP_OBJECT_VALUE(input);
	}

	if (key_type != JSON_OBJECT_END)
		errx(1, "Unexpected object key type");
}

void
gitea_get_issue_summary(
	const char          *owner,
	const char          *repo,
	int                  issue_number,
	ghcli_issue_details *out)
{
	char               *url     = NULL;
	char               *e_owner = NULL;
	char               *e_repo  = NULL;
	ghcli_fetch_buffer  buffer  = {0};
	json_stream         parser  = {0};

	e_owner = ghcli_urlencode(owner);
	e_repo  = ghcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/issues/%d",
		gitea_get_apibase(),
		e_owner, e_repo,
		issue_number);
	ghcli_fetch(url, NULL, &buffer);

	json_open_buffer(&parser, buffer.data, buffer.length);
	json_set_streaming(&parser, true);

	gitea_parse_issue_details(&parser, out);

	json_close(&parser);
	free(url);
	free(e_owner);
	free(e_repo);
	free(buffer.data);
}

void
gitea_submit_issue(
	ghcli_submit_issue_options	 opts,
	ghcli_fetch_buffer			*out)
{
	sn_sv e_owner = ghcli_urlencode_sv(opts.owner);
	sn_sv e_repo  = ghcli_urlencode_sv(opts.repo);

	char *post_fields = sn_asprintf(
		"{ \"title\": \""SV_FMT"\", \"body\": \""SV_FMT"\" }",
		SV_ARGS(opts.title), SV_ARGS(opts.body));
	char *url         = sn_asprintf(
		"%s/repos/"SV_FMT"/"SV_FMT"/issues",
		gitea_get_apibase(),
		SV_ARGS(e_owner),
		SV_ARGS(e_repo));

	ghcli_fetch_with_method("POST", url, post_fields, NULL, out);

	free(e_owner.data);
	free(e_repo.data);
	free(post_fields);
	free(url);
}
