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

#include <gcli/curl.h>
#include <gcli/gitlab/config.h>
#include <gcli/gitlab/forks.h>
#include <gcli/json_util.h>

#include <pdjson/pdjson.h>

#include <templates/gitlab/forks.h>

int
gitlab_get_forks(char const *owner,
                 char const *repo,
                 int const max,
                 gcli_fork_list *const list)
{
	gcli_fetch_buffer   buffer   = {0};
	char               *url      = NULL;
	char               *e_owner  = NULL;
	char               *e_repo   = NULL;
	char               *next_url = NULL;
	struct json_stream  stream   = {0};

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	*list = (gcli_fork_list) {0};

	url = sn_asprintf(
		"%s/projects/%s%%2F%s/forks",
		gitlab_get_apibase(),
		e_owner, e_repo);

	do {
		gcli_fetch(url, &next_url, &buffer);
		json_open_buffer(&stream, buffer.data, buffer.length);
		parse_gitlab_forks(&stream, &list->forks, &list->forks_size);

		json_close(&stream);
		free(buffer.data);
		free(url);
	} while ((url = next_url) && (max == -1 || (int)list->forks_size < max));

	free(next_url);
	free(e_owner);
	free(e_repo);

	return 0;
}

void
gitlab_fork_create(char const *owner, char const *repo, char const *_in)
{
	char              *url       = NULL;
	char              *e_owner   = NULL;
	char              *e_repo    = NULL;
	char              *post_data = NULL;
	sn_sv              in        = SV_NULL;
	gcli_fetch_buffer  buffer    = {0};

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/projects/%s%%2F%s/fork",
		gitlab_get_apibase(),
		e_owner, e_repo);
	if (_in) {
		in = gcli_json_escape(SV((char *)_in));
		post_data = sn_asprintf("{\"namespace_path\":\""SV_FMT"\"}",
		                        SV_ARGS(in));
	}

	gcli_fetch_with_method("POST", url, post_data, NULL, &buffer);
	gcli_print_html_url(buffer);

	free(in.data);
	free(url);
	free(post_data);
	free(e_owner);
	free(e_repo);
	free(buffer.data);
}
