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

#include <gcli/config.h>
#include <gcli/curl.h>
#include <gcli/github/config.h>
#include <gcli/github/repos.h>
#include <gcli/json_util.h>

#include <pdjson/pdjson.h>

#include <templates/github/repos.h>

int
github_get_repos(char const *owner, int const max, gcli_repo_list *const list)
{
	char *url = NULL;
	char *e_owner = NULL;
	gcli_fetch_list_ctx ctx = {
		.listp = &list->repos,
		.sizep = &list->repos_size,
		.max = max,
		.parse = (parsefn)(parse_github_repos),
	};

	e_owner = gcli_urlencode(owner);

	/* Github is a little stupid in that it distinguishes
	 * organizations and users. Thus, we have to find out, whether the
	 * <org> param is a user or an actual organization. */
	url = sn_asprintf("%s/users/%s", gcli_get_apibase(), e_owner);
	if (gcli_curl_test_success(url)) {
		/* it is a user */
		free(url);
		url = sn_asprintf("%s/users/%s/repos",
		                  gcli_get_apibase(),
		                  e_owner);
	} else {
		/* this is an actual organization */
		free(url);
		url = sn_asprintf("%s/orgs/%s/repos",
		                  gcli_get_apibase(),
		                  e_owner);
	}

	free(e_owner);

	return gcli_fetch_list(url, &ctx);
}

int
github_get_own_repos(int const max, gcli_repo_list *const list)
{
	char *url = NULL;
	gcli_fetch_list_ctx ctx = {
		.listp = &list->repos,
		.sizep = &list->repos_size,
		.max = max,
		.parse = (parsefn)(parse_github_repos),
	};

	url = sn_asprintf("%s/user/repos", gcli_get_apibase());

	return gcli_fetch_list(url, &ctx);
}

int
github_repo_delete(char const *owner, char const *repo)
{
	char *url = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	int rc = 0;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf("%s/repos/%s/%s",
	                  gcli_get_apibase(),
	                  e_owner, e_repo);

	rc = gcli_fetch_with_method("DELETE", url, NULL, NULL, NULL);

	free(e_owner);
	free(e_repo);
	free(url);

	return rc;
}

int
github_repo_create(gcli_repo_create_options const *options,
                   gcli_repo *const out)
{
	char *url, *data;
	gcli_fetch_buffer buffer = {0};
	struct json_stream stream = {0};
	sn_sv e_name, e_description;
	int rc = 0;

	/* Request preparation */
	url = sn_asprintf("%s/user/repos", gcli_get_apibase());

	/* JSON-escape repo name and description */
	e_name = gcli_json_escape(options->name);
	e_description = gcli_json_escape(options->description);

	/* Construct payload */
	data = sn_asprintf("{\"name\": \""SV_FMT"\","
	                   " \"description\": \""SV_FMT"\","
	                   " \"private\": %s }",
	                   SV_ARGS(e_name), SV_ARGS(e_description),
	                   gcli_json_bool(options->private));

	/* Fetch and parse result */
	rc = gcli_fetch_with_method("POST", url, data, NULL, out ? &buffer : NULL);

	if (rc == 0 && out) {
		json_open_buffer(&stream, buffer.data, buffer.length);
		parse_github_repo(&stream, out);
		json_close(&stream);
	}

	/* Cleanup */
	free(buffer.data);
	free(e_name.data);
	free(e_description.data);
	free(data);
	free(url);

	return rc;
}
