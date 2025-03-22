/*
 * Copyright 2021-2025 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/github/comments.h>
#include <gcli/github/config.h>
#include <gcli/github/issues.h>
#include <gcli/github/path.h>
#include <gcli/github/repos.h>
#include <gcli/json_gen.h>
#include <gcli/json_util.h>

#include <pdjson/pdjson.h>

#include <templates/github/comments.h>

int
github_perform_submit_comment(struct gcli_ctx *ctx,
                              struct gcli_submit_comment_opts const *const opts)
{
	int rc = 0;
	struct gcli_jsongen gen = {0};
	char *payload = NULL, *url = NULL;

	rc = github_issue_make_url(ctx, &opts->target, &url, "/comments");
	if (rc < 0)
		return rc;

	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "body");
		gcli_jsongen_string(&gen, opts->message);
	}
	gcli_jsongen_end_object(&gen);

	payload = gcli_jsongen_to_string(&gen);
	gcli_jsongen_free(&gen);

	rc = gcli_fetch_with_method(ctx, "POST", url, payload, NULL, NULL);

	gcli_clear_ptr(&payload);
	gcli_clear_ptr(&url);

	return rc;
}

int
github_fetch_comments(struct gcli_ctx *ctx, char *url,
                      struct gcli_comment_list *const out)
{
	struct gcli_fetch_list_ctx fl = {
		.listp = &out->comments,
		.sizep = &out->comments_size,
		.parse = (parsefn)parse_github_comments,
		.max = -1,
	};

	return gcli_fetch_list(ctx, url, &fl);
}

int
github_get_comments(struct gcli_ctx *ctx, struct gcli_path const *const path,
                    struct gcli_comment_list *const out)
{
	char *url = NULL;
	int rc = 0;
	struct gcli_path const *p = path;
	struct gcli_path norm_path = {0};

	/* This routine can be called with paths to PRs too. However,
	 * when called on a PR we get its review comments. We don't
	 * want that. Return its issue comments (which is the
	 * regular discussion) instead.
	 * For this purpose forcefully convert the path to a default
	 * style path and derive the correct url (to the underlying issue)
	 * from that default path. */
	if (path->kind != GCLI_PATH_DEFAULT) {
		rc = github_path_normalise(ctx, path, &norm_path);
		if (rc < 0)
			return rc;

		p = &norm_path;
	}

	/* issue URL from normalised path */
	rc = github_issue_make_url(ctx, p, &url, "/comments");
	if (rc < 0)
		return rc;

	rc = github_fetch_comments(ctx, url, out);

	/* free normalised path if it was used */
	if (p == &norm_path)
		gcli_path_free(&norm_path);

	return rc;
}

static int
github_fetch_comment(struct gcli_ctx *ctx, char const *const url,
                     struct gcli_comment *const out)
{
	struct gcli_fetch_buffer buffer = {0};
	struct json_stream stream = {0};
	int rc = 0;

	rc = gcli_fetch(ctx, url, NULL, &buffer);
	if (rc < 0)
		return rc;

	json_open_buffer(&stream, buffer.data, buffer.length);
	rc = parse_github_comment(ctx, &stream, out);
	json_close(&stream);

	gcli_fetch_buffer_free(&buffer);

	return rc;
}

int
github_get_comment(struct gcli_ctx *ctx, struct gcli_path const *const target,
                   enum comment_target_type target_type, gcli_id comment_id,
                   struct gcli_comment *out)
{
	char *url = NULL;
	int rc = 0;

	(void) target_type; /* target type and id ignored as pull requests are issues on GitHub */

	rc = github_repo_make_url(ctx, target, &url, "/issues/comments/%"PRIid,
	                          comment_id);
	if (rc < 0)
		return rc;

	rc = github_fetch_comment(ctx, url, out);

	gcli_clear_ptr(&url);

	return rc;
}
