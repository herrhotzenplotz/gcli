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
#include <ghcli/json_util.h>
#include <ghcli/gitea/config.h>
#include <ghcli/gitea/pulls.h>

#include <pdjson/pdjson.h>

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
