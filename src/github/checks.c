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

#include <assert.h>

#include <gcli/github/checks.h>
#include <gcli/config.h>
#include <gcli/color.h>
#include <gcli/curl.h>
#include <gcli/json_util.h>

#include <pdjson/pdjson.h>

static void
github_parse_check(struct json_stream *input, gcli_github_check *it)
{
	if (json_next(input) != JSON_OBJECT)
		errx(1, "Expected Check Object");

	while (json_next(input) == JSON_STRING) {
		size_t      len = 0;
		const char *key = json_get_string(input, &len);

		if (strncmp("name", key, len) == 0)
			it->name = get_string(input);
		else if (strncmp("status", key, len) == 0)
			it->status = get_string(input);
		else if (strncmp("conclusion", key, len) == 0)
			it->conclusion = get_string(input);
		else if (strncmp("started_at", key, len) == 0)
			it->started_at = get_string(input);
		else if (strncmp("completed_at", key, len) == 0)
			it->completed_at = get_string(input);
		else if (strncmp("id", key, len) == 0)
			it->id = get_int(input);
		else
			SKIP_OBJECT_VALUE(input);
	}

}

int
github_get_checks(
	const char *owner,
	const char *repo,
	const char *ref,
	int         max,
	gcli_github_check **out)
{
	gcli_fetch_buffer	 buffer	  = {0};
	char				*url	  = NULL;
	char                *next_url = NULL;
	int					 out_size = 0;

	assert(out);

	*out = NULL;

	url = sn_asprintf("%s/repos/%s/%s/commits/%s/check-runs",
					  gcli_get_apibase(),
					  owner, repo, ref);

	do {
		struct json_stream stream = {0};
		enum   json_type   next   = JSON_NULL;

		gcli_fetch(url, &next_url, &buffer);
		json_open_buffer(&stream, buffer.data, buffer.length);
		json_set_streaming(&stream, 1);

		gcli_json_advance(&stream, "{sis", "total_count", "check_runs");

		next = json_next(&stream);
		if (next != JSON_ARRAY)
			errx(1, "error: expected array of checks");

		while (json_peek(&stream) != JSON_ARRAY_END) {
			gcli_github_check *it = NULL;

			*out = realloc(*out, sizeof(gcli_github_check) * (out_size + 1));
			it = &(*out)[out_size++];

			memset(it, 0, sizeof(*it));
			github_parse_check(&stream, it);

			if (out_size == max) {
				/* corner case: if this gets hit, we would never free
				 * the next_url. it is initialized to NULL by default,
				 * so we wouldn't get an invalid free with a garbage
				 * pointer */
				free(next_url);
				break;
			}
		}

		json_close(&stream);
		free(url);
		free(buffer.data);
	} while ((url = next_url) && ((int)(out_size) < max || max < 0));

	return out_size;
}

void
github_print_checks(gcli_github_check *checks, int checks_size)
{
	printf("%10.10s  %10.10s  %10.10s  %16.16s  %16.16s  %-s\n",
		   "ID", "STATUS", "CONCLUSION", "STARTED", "COMPLETED", "NAME");

	for (int i = 0; i < checks_size; ++i) {
		printf("%10ld  %10.10s  %s%10.10s%s  %16.16s  %16.16s  %-s\n",
			   checks[i].id,
			   checks[i].status,
			   gcli_state_color_str(checks[i].conclusion),
			   checks[i].conclusion,
			   gcli_resetcolor(),
			   checks[i].started_at,
			   checks[i].completed_at,
			   checks[i].name);
	}
}

void
github_free_checks(
	gcli_github_check *checks,
	int                 checks_size)
{
	for (int i = 0; i < checks_size; ++i) {
		free(checks[i].name);
		free(checks[i].status);
		free(checks[i].conclusion);
		free(checks[i].started_at);
		free(checks[i].completed_at);
	}

	free(checks);
}

void
github_checks(
	const char	*owner,
	const char	*repo,
	const char	*ref,
	int			 max)
{
	gcli_github_check	*checks		 = NULL;
	int					 checks_size = 0;

	checks_size = github_get_checks(owner, repo, ref, max, &checks);
	github_print_checks(checks, checks_size);
	github_free_checks(checks, checks_size);
}
