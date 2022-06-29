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

#include <gcli/curl.h>
#include <gcli/gitlab/config.h>
#include <gcli/gitlab/review.h>
#include <gcli/json_util.h>

#include <pdjson/pdjson.h>
#include <sn/sn.h>

#include <stdlib.h>

static void
gitlab_parse_review_note(struct json_stream *input, gcli_pr_review *out)
{
	if (json_next(input) != JSON_OBJECT)
		errx(1, "Expected object");

	enum json_type key_type;
	while ((key_type = json_next(input)) == JSON_STRING) {
		size_t      len = 0;
		const char *key = json_get_string(input, &len);

		if (strncmp("created_at", key, len) == 0)
			out->date = get_string(input);
		else if (strncmp("body", key, len) == 0)
			out->body = get_string(input);
		else if (strncmp("author", key, len) == 0)
			out->author = get_user(input);
		else if (strncmp("id", key, len) == 0)
			out->id = sn_asprintf("%ld", get_int(input));
		else
			SKIP_OBJECT_VALUE(input);
	}

	if (key_type != JSON_OBJECT_END)
		errx(1, "Expected object end");

	/* Gitlab works a little different with comments on merge
	 * requests. Set it to 0. */
	out->comments_size = 0;
	out->comments      = NULL;
	out->state         = NULL;
}

size_t
gitlab_review_get_reviews(
	const char       *owner,
	const char       *repo,
	int               pr,
	gcli_pr_review **out)
{
	gcli_fetch_buffer  buffer   = {0};
	struct json_stream  stream   = {0};
	char               *url      = NULL;
	char               *next_url = NULL;
	int                 size     = 0;

	url = sn_asprintf(
		"%s/projects/%s%%2F%s/merge_requests/%d/notes?sort=asc",
		gitlab_get_apibase(), owner, repo, pr);

	do {
		gcli_fetch(url, &next_url, &buffer);

		json_open_buffer(&stream, buffer.data, buffer.length);
		json_set_streaming(&stream, 1);

		if (json_next(&stream) != JSON_ARRAY)
			errx(1, "Expected array");

		while (json_peek(&stream) == JSON_OBJECT) {
			*out = realloc(*out, sizeof(gcli_pr_review) * (size + 1));
			gcli_pr_review *it  = &(*out)[size++];
			gitlab_parse_review_note(&stream, it);
		}

		if (json_next(&stream) != JSON_ARRAY_END)
			errx(1, "Expected array end");

		json_close(&stream);
		free(url);
		free(buffer.data);
	} while ((url = next_url)); /* I hope this doesn't cause any issues */

	free(next_url);

	return size;
}
