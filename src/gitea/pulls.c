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
#include <ghcli/gitea/pulls.h>
#include <ghcli/github/pulls.h>
#include <ghcli/json_util.h>

#include <pdjson/pdjson.h>

static char *
get_branch_label(json_stream *input)
{
	enum json_type	 key_type;
	char			*label = NULL;

	if (json_next(input) != JSON_OBJECT)
		errx(1, "expected branch object");

	while ((key_type = json_next(input)) == JSON_STRING) {
		size_t       len = 0;
		const char  *key = json_get_string(input, &len);

		if (strncmp("label", key, len) == 0)
			label = get_string(input);
		else
			SKIP_OBJECT_VALUE(input);
	}

	if (key_type != JSON_OBJECT_END)
		errx(1, "unexpected key type in branch object");

	return label;
}

static void
gitea_parse_pull(struct json_stream *input, ghcli_pull *it)
{
	if (json_next(input) != JSON_OBJECT)
		errx(1, "Expected Issue Object");

	enum json_type key_type;
	while ((key_type = json_next(input)) == JSON_STRING) {
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
		else if (strncmp("merged_at", key, len) == 0)
			it->merged = json_next(input) == JSON_STRING;
		else if (strncmp("user", key, len) == 0)
			it->creator = get_user(input);
		else
			SKIP_OBJECT_VALUE(input);
	}
}

int
gitea_get_pulls(
	const char  *owner,
	const char  *repo,
	bool         all,
	int          max,
	ghcli_pull **out)
{
	char	*url	  = NULL;
	char	*next_url = NULL;
	char	*e_owner  = NULL;
	char	*e_repo	  = NULL;
	int		 out_size = 0;

	*out = NULL;

	e_owner = ghcli_urlencode(owner);
	e_repo  = ghcli_urlencode(repo);

	url = sn_asprintf("%s/repos/%s/%s/pulls%s",
					  gitea_get_apibase(),
					  e_owner, e_repo,
					  all ? "?state=all" : "");

	do {
		ghcli_fetch_buffer	buffer = {0};
		struct json_stream	stream = {0};
		enum   json_type	next   = JSON_NULL;

		ghcli_fetch(url, &next_url, &buffer);
		json_open_buffer(&stream, buffer.data, buffer.length);

		/* Expect an array */
		next = json_next(&stream);
		if (next != JSON_ARRAY)
			errx(1, "error: expected a json array in api response");

		while (json_peek(&stream) == JSON_OBJECT) {
			*out = realloc(*out, sizeof(**out) * (out_size + 1));

			ghcli_pull *it = &(*out)[out_size++];
			gitea_parse_pull(&stream, it);

			if (out_size == max)
				break;
		}

		json_close(&stream);
		free(url);
		free(buffer.data);
	} while ((url = next_url) && (max == -1 || out_size < max));

	free(next_url);
	free(e_owner);
	free(e_repo);
	return out_size;
}

static void
gitea_parse_pull_head(json_stream *input, ghcli_pull_summary *out)
{
	enum json_type key_type;
	const char *key;

	json_next(input);

	while ((key_type = json_next(input)) == JSON_STRING) {
		size_t len;
		key = json_get_string(input, &len);
		if (strncmp("sha", key, len) == 0)
			out->head_sha = get_string(input);
		else if (strncmp("label", key, len) == 0)
			out->head_label = get_string(input);
		else
			SKIP_OBJECT_VALUE(input);
	}

	if (key_type != JSON_OBJECT_END)
		errx(1, "Unexpected object key type");
}

static void
gitea_pull_parse_summary(json_stream *input, ghcli_pull_summary *out)
{
	enum json_type key_type;
	const char *key;

	json_next(input);

	while ((key_type = json_next(input)) == JSON_STRING) {
		size_t len;
		key = json_get_string(input, &len);

		if (strncmp("title", key, len) == 0)
			out->title = get_string(input);
		else if (strncmp("state", key, len) == 0)
			out->state = get_string(input);
		else if (strncmp("body", key, len) == 0)
			out->body = get_string(input);
		else if (strncmp("created_at", key, len) == 0)
			out->created_at = get_string(input);
		else if (strncmp("number", key, len) == 0)
			out->number = get_int(input);
		else if (strncmp("id", key, len) == 0)
			out->id = get_int(input);
		else if (strncmp("commits", key, len) == 0)
			out->commits = get_int(input);
		else if (strncmp("labels", key, len) == 0)
			out->labels_size = ghcli_read_label_list(input, &out->labels);
		else if (strncmp("comments", key, len) == 0)
			out->comments = get_int(input);
		else if (strncmp("additions", key, len) == 0)
			out->additions = get_int(input);
		else if (strncmp("deletions", key, len) == 0)
			out->deletions = get_int(input);
		else if (strncmp("changed_files", key, len) == 0)
			out->changed_files = get_int(input);
		else if (strncmp("merged", key, len) == 0)
			out->merged = get_bool(input);
		else if (strncmp("mergeable", key, len) == 0)
			out->mergeable = get_bool(input);
		else if (strncmp("draft", key, len) == 0)
			out->draft = get_bool(input);
		else if (strncmp("user", key, len) == 0)
			out->author = get_user(input);
		else if (strncmp("head", key, len) == 0)
			gitea_parse_pull_head(input, out);
		else if (strncmp("base", key, len) == 0)
			out->base_label = get_branch_label(input);
		else
			SKIP_OBJECT_VALUE(input);
	}

	if (key_type != JSON_OBJECT_END)
		errx(1, "Unexpected object key type");
}

void
gitea_get_pull_summary(
	const char			*owner,
	const char			*repo,
	int					 pr_number,
	ghcli_pull_summary	*out)
{
	json_stream         stream      = {0};
	ghcli_fetch_buffer  json_buffer = {0};
	char               *url         = NULL;
	char               *e_owner     = NULL;
	char               *e_repo      = NULL;

	e_owner = ghcli_urlencode(owner);
	e_repo  = ghcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/pulls/%d",
		gitea_get_apibase(),
		e_owner, e_repo, pr_number);
	ghcli_fetch(url, NULL, &json_buffer);

	json_open_buffer(&stream, json_buffer.data, json_buffer.length);

	gitea_pull_parse_summary(&stream, out);

	json_close(&stream);
	free(url);
	free(e_owner);
	free(e_repo);
	free(json_buffer.data);
}

static void
gitea_parse_commit_author_field(json_stream *input, ghcli_commit *it)
{
	enum json_type key_type;
	const char *key;

	json_next(input);

	while ((key_type = json_next(input)) == JSON_STRING) {
		size_t len;
		key = json_get_string(input, &len);

		if (strncmp(key, "name", len) == 0)
			it->author = get_string(input);
		else if (strncmp(key, "email", len) == 0)
			it->email = get_string(input);
		else if (strncmp(key, "date", len) == 0)
			it->date = get_string(input);
		else
			SKIP_OBJECT_VALUE(input);
	}

	if (key_type != JSON_OBJECT_END)
		errx(1, "Unexpected non-string object key");
}

static void
gitea_parse_commit_commit_field(json_stream *input, ghcli_commit *it)
{
	enum json_type key_type;
	const char *key;

	json_next(input);

	while ((key_type = json_next(input)) == JSON_STRING) {
		size_t len;
		key = json_get_string(input, &len);

		if (strncmp(key, "message", len) == 0)
			it->message = get_string(input);
		else if (strncmp(key, "author", len) == 0)
			gitea_parse_commit_author_field(input, it);
		else
			SKIP_OBJECT_VALUE(input);
	}

	if (key_type != JSON_OBJECT_END)
		errx(1, "Unexpected non-string object key");
}

static void
gitea_parse_commit(json_stream *input, ghcli_commit *it)
{
	enum json_type key_type;
	const char *key;

	json_next(input);

	while ((key_type = json_next(input)) == JSON_STRING) {
		size_t len;
		key = json_get_string(input, &len);

		if (strncmp(key, "sha", len) == 0)
			it->sha = get_string(input);
		else if (strncmp(key, "commit", len) == 0)
			gitea_parse_commit_commit_field(input, it);
		else
			SKIP_OBJECT_VALUE(input);
	}

	if (key_type != JSON_OBJECT_END)
		errx(1, "Unexpected non-string object key");
}

int
gitea_get_pull_commits(
	const char    *owner,
	const char    *repo,
	int            pr_number,
	ghcli_commit **out)
{
	char              *url         = NULL;
	char              *next_url    = NULL;
	char              *e_owner     = NULL;
	char              *e_repo      = NULL;
	int                count       = 0;
	json_stream        stream      = {0};
	ghcli_fetch_buffer json_buffer = {0};

	e_owner = ghcli_urlencode(owner);
	e_repo  = ghcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/pulls/%d/commits",
		gitea_get_apibase(),
		e_owner, e_repo, pr_number);

	do {
		ghcli_fetch(url, &next_url, &json_buffer);
		json_open_buffer(&stream, json_buffer.data, json_buffer.length);
		json_set_streaming(&stream, true);

		enum json_type next_token = json_next(&stream);

		while ((next_token = json_peek(&stream)) != JSON_ARRAY_END) {
			if (next_token != JSON_OBJECT)
				errx(1, "Unexpected non-object in commit list");

			*out = realloc(*out, (count + 1) * sizeof(ghcli_commit));
			ghcli_commit *it = &(*out)[count];
			gitea_parse_commit(&stream, it);
			count += 1;
		}

		json_close(&stream);
		free(json_buffer.data);
		free(url);
	} while ((url = next_url));

	free(e_owner);
	free(e_repo);

	return count;
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
