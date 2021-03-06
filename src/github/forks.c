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
#include <gcli/github/forks.h>
#include <gcli/json_util.h>

#include <pdjson/pdjson.h>

static void
parse_fork(struct json_stream *input, gcli_fork *out)
{
	enum json_type  key_type = JSON_NULL;
	const char     *key      = NULL;

	if (json_next(input) != JSON_OBJECT)
		errx(1, "Expected an object for a fork");

	while ((key_type = json_next(input)) == JSON_STRING) {
		size_t len;
		key = json_get_string(input, &len);

		if (strncmp("full_name", key, len) == 0) {
			out->full_name = get_sv(input);
		} else if (strncmp("owner", key, len) == 0) {
			char *user = get_user(input);
			out->owner = SV(user);
		} else if (strncmp("created_at", key, len) == 0) {
			out->date = get_sv(input);
		} else if (strncmp("forks_count", key, len) == 0) {
			out->forks = get_int(input);
		} else {
			SKIP_OBJECT_VALUE(input);
		}
	}

	if (key_type != JSON_OBJECT_END)
		errx(1, "Fork object not closed");
}

int
github_get_forks(
	const char  *owner,
	const char  *repo,
	int          max,
	gcli_fork **out)
{
	gcli_fetch_buffer  buffer   = {0};
	char               *url      = NULL;
	char               *e_owner  = NULL;
	char               *e_repo   = NULL;
	char               *next_url = NULL;
	enum   json_type    next     = JSON_NULL;
	struct json_stream  stream   = {0};
	int                 size     = 0;

	*out = NULL;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/forks",
		gcli_get_apibase(),
		e_owner, e_repo);

	do {
		gcli_fetch(url, &next_url, &buffer);

		json_open_buffer(&stream, buffer.data, buffer.length);
		json_set_streaming(&stream, 1);

		// TODO: Poor error message
		if ((next = json_next(&stream)) != JSON_ARRAY)
			errx(1,
				 "Expected array in response from API "
				 "but got something else instead");

		while ((next = json_peek(&stream)) != JSON_ARRAY_END) {
			*out = realloc(*out, sizeof(gcli_fork) * (size + 1));
			gcli_fork *it = &(*out)[size++];
			parse_fork(&stream, it);

			if (size == max)
				break;
		}

		json_close(&stream);
		free(buffer.data);
		free(url);
	} while ((url = next_url) && (max == -1 || size < max));

	free(next_url);
	free(e_owner);
	free(e_repo);

	return size;
}

void
github_fork_create(const char *owner, const char *repo, const char *_in)
{
	char               *url       = NULL;
	char               *e_owner   = NULL;
	char               *e_repo    = NULL;
	char               *post_data = NULL;
	sn_sv               in        = SV_NULL;
	gcli_fetch_buffer  buffer    = {0};

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/forks",
		gcli_get_apibase(),
		e_owner, e_repo);
	if (_in) {
		in        = gcli_json_escape(SV((char *)_in));
		post_data = sn_asprintf("{\"organization\":\""SV_FMT"\"}",
					SV_ARGS(in));
	}

	gcli_fetch_with_method("POST", url, post_data, NULL, &buffer);
	gcli_print_html_url(buffer);

	free(in.data);
	free(url);
	free(e_owner);
	free(e_repo);
	free(post_data);
	free(buffer.data);
}
