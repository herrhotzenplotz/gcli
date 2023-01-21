/*
 * Copyright 2023 Nico Sonack <nsonack@herrhotzenplotz.de>
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
#include <gcli/gitlab/milestones.h>

#include <templates/gitlab/milestones.h>

#include <pdjson/pdjson.h>

int
gitlab_get_milestones(char const *owner,
                      char const *repo,
                      int max,
                      gcli_milestone_list *const out)
{
	char *url, *next_url = NULL;
	char *e_owner, *e_repo;
	gcli_fetch_buffer buffer = {0};
	json_stream stream = {0};

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/milestones",
	                  gitlab_get_apibase(), e_owner, e_repo);

	do {
		gcli_fetch(url, &next_url, &buffer);
		json_open_buffer(&stream, buffer.data, buffer.length);

		parse_gitlab_milestones(&stream, &out->milestones, &out->milestones_size);

		free(buffer.data);
		free(url);
		json_close(&stream);
	} while ((url = next_url) && (max == -1 || (int)out->milestones_size < max));

	free(next_url);
	free(e_owner);
	free(e_repo);

	return 0;
}

int
gitlab_get_milestone(char const *owner,
                     char const *repo,
                     int const milestone,
                     gcli_milestone *const out)
{
	char *url, *e_owner, *e_repo;
	gcli_fetch_buffer buffer = {0};
	json_stream stream = {0};

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/milestones/%d",
	                  gitlab_get_apibase(), e_owner, e_repo, milestone);

	gcli_fetch(url, NULL, &buffer);
	json_open_buffer(&stream, buffer.data, buffer.length);

	parse_gitlab_milestone(&stream, out);

	json_close(&stream);
	free(buffer.data);
	free(url);
	free(e_owner);
	free(e_repo);

	return 0;
}
