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

#include <gcli/github/review.h>
#include <gcli/config.h>
#include <gcli/json_util.h>

#include <pdjson/pdjson.h>

static int
github_parse_review_comment(gcli_ctx *ctx, json_stream *input,
                            gcli_pr_review_comment *it)
{
	if (json_next(input) != JSON_OBJECT)
		return gcli_error(ctx, "%s: expected review comment object",
		                  __func__);

	enum json_type key_type;
	while ((key_type = json_next(input)) == JSON_STRING) {
		size_t len = 0;
		char const *key = json_get_string(input, &len);

		if (strncmp("bodyText", key, len) == 0) {
			if (get_string(ctx, input, &it->body) < 0)
				return -1;

		} else if (strncmp("id", key, len) == 0) {
			if (get_string(ctx, input, &it->id) < 0)
				return -1;

		} else if (strncmp("createdAt", key, len) == 0) {
			if (get_string(ctx, input, &it->date) < 0)
				return -1;

		} else if (strncmp("author", key, len) == 0) {
			if (get_user(ctx, input, &it->author) < 0)
				return -1;

		} else if (strncmp("diffHunk", key, len) == 0) {
			if (get_string(ctx, input, &it->diff) < 0)
				return -1;

		} else if (strncmp("path", key, len) == 0) {
			if (get_string(ctx, input, &it->path) < 0)
				return -1;

		} else if (strncmp("originalPosition", key, len) == 0) {
			if (get_int(ctx, input, &it->original_position) < 0)
				return -1;

		} else {
			SKIP_OBJECT_VALUE(input);
		}
	}

	return 0;
}

static int
github_parse_review_comments(gcli_ctx *ctx, json_stream *input,
                             gcli_pr_review *it)
{
	if (gcli_json_advance(ctx, input, "{s[", "nodes") < 0)
		return -1;

	while (json_peek(input) == JSON_OBJECT) {
		it->comments = realloc(
			it->comments,
			sizeof(*it->comments) * (it->comments_size + 1));
		gcli_pr_review_comment *comment = &it->comments[it->comments_size++];
		*comment = (gcli_pr_review_comment) {0};

		if (github_parse_review_comment(ctx, input, comment) < 0)
			return -1;
	}

	if (gcli_json_advance(ctx, input, "]}") < 0)
		return -1;

	return 0;
}

static int
github_parse_review_header(gcli_ctx *ctx, json_stream *input,
                           gcli_pr_review *it)
{
	if (json_next(input) != JSON_OBJECT)
		return gcli_error(ctx, "%s: expected an object", __func__);

	enum json_type key_type;
	while ((key_type = json_next(input)) == JSON_STRING) {
		size_t      len = 0;
		char const *key = json_get_string(input, &len);

		if (strncmp("bodyText", key, len) == 0) {
			if (get_string(ctx, input, &it->body) < 0)
				return -1;

		} else if (strncmp("state", key, len) == 0) {
			if (get_string(ctx, input, &it->state) < 0)
				return -1;

		} else if (strncmp("id", key, len) == 0) {
			if (get_string(ctx, input, &it->id) < 0)
				return -1;

		} else if (strncmp("createdAt", key, len) == 0) {
			if (get_string(ctx, input, &it->date) < 0)
				return -1;
		} else if (strncmp("author", key, len) == 0) {
			if (get_user(ctx, input, &it->author) < 0)
				return -1;
		} else if (strncmp("comments", key, len) == 0) {
			if (github_parse_review_comments(ctx, input, it) < 0)
				return -1;
		} else {
			SKIP_OBJECT_VALUE(input);
		}
	}

	return 0;
}

static char const *get_reviews_fmt =
	"query {"
	"  repository(owner: \"%s\", name: \"%s\") {"
	"    pullRequest(number: %d) {"
	"      reviews(first: 10) {"
	"        nodes {"
	"          author {"
	"            login"
	"          }"
	"          bodyText"
	"          id"
	"          createdAt"
	"          state"
	"          comments(first: 10) {"
	"            nodes {"
	"              bodyText"
	"              diffHunk"
	"              path"
	"              originalPosition"
	"              author {"
	"                login"
	"              }"
	"              state"
	"              createdAt"
	"              id"
	"            }"
	"          }"
	"        }"
	"      }"
	"    }"
	"  }"
	"}";

int
github_review_get_reviews(gcli_ctx *ctx, char const *owner, char const *repo,
                          int const pr, gcli_pr_review_list *const out)
{
	gcli_fetch_buffer buffer = {0};
	char *url = NULL;
	char *query = NULL;
	sn_sv query_escaped = {0};
	char *post_data = NULL;
	struct json_stream stream = {0};
	enum json_type next = JSON_NULL;
	int rc = 0;

	url = sn_asprintf("%s/graphql", gcli_get_apibase(ctx));
	query = sn_asprintf(get_reviews_fmt, owner, repo, pr);
	query_escaped = gcli_json_escape(SV(query));
	post_data = sn_asprintf("{\"query\": \""SV_FMT"\"}",
	                        SV_ARGS(query_escaped));

	rc = gcli_fetch_with_method(ctx, "POST", url, post_data, NULL, &buffer);
	free(url);
	free(query);
	free(query_escaped.data);
	free(post_data);

	if (rc < 0)
		goto error_fetch;

	json_open_buffer(&stream, buffer.data, buffer.length);
	json_set_streaming(&stream, true);

	rc = gcli_json_advance(ctx, &stream, "{s{s{s{s{s",
	                       "data", "repository", "pullRequest",
	                       "reviews", "nodes");
	if (rc < 0)
		goto error_json;

	next = json_next(&stream);
	if (next != JSON_ARRAY) {
		rc = gcli_error(ctx, "expected json array for review list");
		goto error_json;
	}

	while ((next = json_peek(&stream)) == JSON_OBJECT) {
		out->reviews = realloc(out->reviews,
		                       sizeof(gcli_pr_review) * (out->reviews_size + 1));
		gcli_pr_review *it = &out->reviews[out->reviews_size];

		*it = (gcli_pr_review) {0};

		rc = github_parse_review_header(ctx, &stream, it);
		if (rc < 0)
			goto error_json;

		out->reviews_size++;
	}

	if (json_next(&stream) != JSON_ARRAY_END) {
		rc = gcli_error(ctx, "expected end of json array");
		goto error_json;
	}

	rc = gcli_json_advance(ctx, &stream, "}}}}}");

error_json:

	json_close(&stream);
error_fetch:
	free(buffer.data);

	return rc;
}
