/*
 * Copyright 2022-2025 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/cmd/colour.h>
#include <gcli/cmd/table.h>
#include <gcli/curl.h>
#include <gcli/github/checks.h>
#include <gcli/github/checks.h>
#include <gcli/github/path.h>
#include <gcli/github/repos.h>
#include <gcli/json_util.h>

#include <templates/github/checks.h>

#include <pdjson.h>

int
github_get_checks(struct gcli_ctx *ctx, struct gcli_path const *const path,
                  char const *ref, int const max, struct github_check_list *const out)
{
	struct gcli_fetch_buffer buffer = {0};
	struct gcli_path norm_path = {0};
	char *url = NULL, *next_url = NULL;
	int rc = 0;

	assert(out);

	/* we must normalise these paths to default paths. otherwise we'd get
	 * bugs when a URL path to some item is passed in. */
	rc = github_path_normalise(ctx, path, &norm_path);
	if (rc < 0)
		return rc;

	rc = github_repo_make_url(ctx, path, &url, "/commits/%s/check-runs", ref);
	if (rc < 0)
		return rc;

	do {
		rc = gcli_fetch(ctx, url, &next_url, &buffer);
		if (rc == 0) {
			struct json_stream stream = {0};

			json_open_buffer(&stream, buffer.data, buffer.length);
			parse_github_checks(ctx, &stream, out);
			json_close(&stream);
		}

		gcli_clear_ptr(&url);
		gcli_fetch_buffer_free(&buffer);

		if (rc < 0)
			break;
	} while ((url = next_url) && ((int)(out->checks_size) < max || max < 0));

	/* TODO: don't leak list on error */
	gcli_clear_ptr(&next_url);

	gcli_path_free(&norm_path);

	return rc;
}

void
gcli_github_check_free(struct gcli_github_check *check)
{
	gcli_clear_ptr(&check->name);
	gcli_clear_ptr(&check->status);
	gcli_clear_ptr(&check->conclusion);
	gcli_clear_ptr(&check->started_at);
	gcli_clear_ptr(&check->completed_at);
}

void
github_free_checks(struct github_check_list *const list)
{
	for (size_t i = 0; i < list->checks_size; ++i) {
		gcli_github_check_free(&list->checks[i]);
	}

	gcli_clear_ptr(&list->checks);
	list->checks_size = 0;
}
