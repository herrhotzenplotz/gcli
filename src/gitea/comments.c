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
#include <ghcli/gitea/comments.h>
#include <ghcli/gitea/config.h>
#include <ghcli/json_util.h>

static void
gitea_parse_comment(json_stream *input, ghcli_comment *it)
{
	if (json_next(input) != JSON_OBJECT)
		errx(1, "Expected Comment Object");

	enum json_type key_type;
	while ((key_type = json_next(input)) == JSON_STRING) {
		size_t      len = 0;
		const char *key = json_get_string(input, &len);

		if (strncmp("created_at", key, len) == 0)
			it->date = get_string(input);
		else if (strncmp("body", key, len) == 0)
			it->body = get_string(input);
		else if (strncmp("user", key, len) == 0)
			it->author = get_user(input);
		else
			SKIP_OBJECT_VALUE(input);
	}
}

int
gitea_get_comments(
	const char     *owner,
	const char     *repo,
	int             issue,
	ghcli_comment **out)
{
	char				*e_owner  = NULL;
	char				*e_repo	  = NULL;
	char				*url	  = NULL;
	char				*next_url = NULL;
	int					 out_size = 0;
	ghcli_fetch_buffer	 buffer	  = {0};
	struct json_stream	 stream	  = {0};

	e_owner = ghcli_urlencode(owner);
	e_repo  = ghcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/issues/%d/comments",
		gitea_get_apibase(),
		e_owner, e_repo, issue);


	do {
		ghcli_fetch(url, &next_url, &buffer);
		json_open_buffer(&stream, buffer.data, buffer.length);
		json_set_streaming(&stream, true);

		enum json_type next_token = json_next(&stream);

		while ((next_token = json_peek(&stream)) != JSON_ARRAY_END) {
			if (next_token != JSON_OBJECT)
				errx(1, "Unexpected non-object in comment list");

			*out = realloc(*out, (out_size + 1) * sizeof(ghcli_comment));
			ghcli_comment *it = &(*out)[out_size];
			gitea_parse_comment(&stream, it);
			out_size += 1;
		}

		json_close(&stream);
		free(buffer.data);
		free(url);
	} while ((url = next_url));

	free(e_owner);
	free(e_repo);
	free(url);
	return out_size;
}
