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
#include <gcli/gitlab/snippets.h>
#include <gcli/json_util.h>
#include <gcli/cmd/table.h>

#include <pdjson/pdjson.h>
#include <sn/sn.h>

#include <stdlib.h>

#include <templates/gitlab/snippets.h>

void
gcli_snippets_free(gcli_snippet_list *const list)
{
	for (size_t i = 0; i < list->snippets_size; ++i) {
		free(list->snippets[i].title);
		free(list->snippets[i].filename);
		free(list->snippets[i].date);
		free(list->snippets[i].author);
		free(list->snippets[i].visibility);
		free(list->snippets[i].raw_url);
	}

	free(list->snippets);

	list->snippets = NULL;
	list->snippets_size = 0;
}

int
gcli_snippets_get(gcli_ctx *ctx, int const max, gcli_snippet_list *const out)
{
	char *url = NULL;

	gcli_fetch_list_ctx fl = {
		.listp = &out->snippets,
		.sizep = &out->snippets_size,
		.max = max,
		.parse = (parsefn)(parse_gitlab_snippets),
	};

	*out = (gcli_snippet_list) {0};
	url = sn_asprintf("%s/snippets", gcli_get_apibase(ctx));

	return gcli_fetch_list(ctx, url, &fl);
}

int
gcli_snippet_delete(gcli_ctx *ctx, char const *snippet_id)
{
	int rc = 0;
	char *url;

	url = sn_asprintf("%s/snippets/%s", gcli_get_apibase(ctx), snippet_id);
	rc = gcli_fetch_with_method(ctx, "DELETE", url, NULL, NULL, NULL);

	free(url);

	return rc;
}

int
gcli_snippet_get(gcli_ctx *ctx, char const *snippet_id, FILE *stream)
{
	int rc = 0;
	char *url = sn_asprintf("%s/snippets/%s/raw",
	                        gcli_get_apibase(ctx),
	                        snippet_id);
	rc = gcli_curl(ctx, stream, url, NULL);
	free(url);

	return rc;
}
