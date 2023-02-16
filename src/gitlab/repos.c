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

#include <gcli/gitlab/config.h>
#include <gcli/gitlab/repos.h>
#include <gcli/json_util.h>

#include <pdjson/pdjson.h>
#include <sn/sn.h>

#include <templates/gitlab/repos.h>

void
gitlab_get_repo(char const *owner,
                char const *repo,
                gcli_repo *const out)
{
	/* GET /projects/:id */
	char              *url     = NULL;
	gcli_fetch_buffer  buffer  = {0};
	json_stream        stream  = {0};
	char              *e_owner = {0};
	char              *e_repo  = {0};

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/projects/%s%%2F%s",
		gitlab_get_apibase(),
	    e_owner, e_repo);

	gcli_fetch(url, NULL, &buffer);
	json_open_buffer(&stream, buffer.data, buffer.length);

	parse_gitlab_repo(&stream, out);

	json_close(&stream);
	free(buffer.data);
	free(e_owner);
	free(e_repo);
	free(url);
}

int
gitlab_get_repos(char const *owner,
                 int const max,
                 gcli_repo_list *const list)
{
	char              *url      = NULL;
	char              *next_url = NULL;
	char              *e_owner  = NULL;
	gcli_fetch_buffer  buffer   = {0};
	json_stream        stream   = {0};

	e_owner = gcli_urlencode(owner);

	url = sn_asprintf("%s/users/%s/projects", gitlab_get_apibase(), e_owner);

	do {
		gcli_fetch(url, &next_url, &buffer);

		json_open_buffer(&stream, buffer.data, buffer.length);

		parse_gitlab_repos(&stream, &list->repos, &list->repos_size);

		free(url);
		free(buffer.data);
		json_close(&stream);
	} while ((url = next_url) && (max == -1 || (int)list->repos_size < max));

	free(url);
	free(e_owner);

	return 0;
}

int
gitlab_get_own_repos(int const max, gcli_repo_list *const out)
{
	char  *_account = NULL;
	sn_sv  account  = {0};
	int    n;

	account = gitlab_get_account();
	if (!account.length)
		errx(1, "error: gitlab.account is not set");

	_account = sn_sv_to_cstr(account);

	n = gitlab_get_repos(_account, max, out);

	free(_account);

	return n;
}

void
gitlab_repo_delete(char const *owner, char const *repo)
{
	char              *url     = NULL;
	char              *e_owner = NULL;
	char              *e_repo  = NULL;
	gcli_fetch_buffer  buffer  = {0};

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s",
	                  gitlab_get_apibase(),
	                  e_owner, e_repo);

	gcli_fetch_with_method("DELETE", url, NULL, NULL, &buffer);

	free(buffer.data);
	free(url);
	free(e_owner);
	free(e_repo);
}

gcli_repo *
gitlab_repo_create(gcli_repo_create_options const *options) /* Options descriptor */
{
	gcli_repo         *repo;
	char              *url, *data;
	gcli_fetch_buffer  buffer = {0};
	json_stream        stream = {0};

	/* Will be freed by the caller with gcli_repos_free */
	repo = calloc(1, sizeof(gcli_repo));

	/* Request preparation */
	url = sn_asprintf("%s/projects", gitlab_get_apibase());
	/* TODO: escape the repo name and the description */
	data = sn_asprintf("{\"name\": \""SV_FMT"\","
	                   " \"description\": \""SV_FMT"\","
	                   " \"visibility\": \"%s\" }",
	                   SV_ARGS(options->name),
	                   SV_ARGS(options->description),
	                   options->private ? "private" : "public");

	/* Fetch and parse result */
	gcli_fetch_with_method("POST", url, data, NULL, &buffer);
	json_open_buffer(&stream, buffer.data, buffer.length);
	parse_gitlab_repo(&stream, repo);

	/* Cleanup */
	json_close(&stream);
	free(buffer.data);
	free(data);
	free(url);

	return repo;
}
