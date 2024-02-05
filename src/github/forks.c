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
#include <gcli/github/config.h>
#include <gcli/github/forks.h>
#include <gcli/json_util.h>

#include <pdjson/pdjson.h>

#include <templates/github/forks.h>

int
github_get_forks(struct  gcli_ctx *ctx, char const *owner, char const *repo,
                 int const max, struct gcli_fork_list *const list)
{
	char *url = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;

	struct gcli_fetch_list_ctx fl = {
		.listp = &list->forks,
		.sizep = &list->forks_size,
		.max = max,
		.parse = (parsefn)(parse_github_forks),
	};

	*list = (struct gcli_fork_list) {0};

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/forks",
		gcli_get_apibase(ctx),
		e_owner, e_repo);

	free(e_owner);
	free(e_repo);

	return gcli_fetch_list(ctx, url, &fl);
}

int
github_fork_create(struct gcli_ctx *ctx, char const *owner, char const *repo,
                   char const *_in)
{
	char *url = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	char *post_data = NULL;
	sn_sv in = SV_NULL;
	int rc = 0;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/repos/%s/%s/forks",
	                  gcli_get_apibase(ctx),
	                  e_owner, e_repo);
	if (_in) {
		in = gcli_json_escape(SV((char *)_in));
		post_data = sn_asprintf("{\"organization\":\""SV_FMT"\"}",
		                        SV_ARGS(in));
	}

	rc = gcli_fetch_with_method(ctx, "POST", url, post_data, NULL, NULL);

	free(in.data);
	free(url);
	free(e_owner);
	free(e_repo);
	free(post_data);

	return rc;
}
