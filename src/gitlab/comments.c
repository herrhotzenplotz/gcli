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

#include <gcli/gitlab/comments.h>
#include <gcli/gitlab/config.h>
#include <gcli/json_util.h>

#include <templates/gitlab/comments.h>

int
gitlab_perform_submit_comment(gcli_ctx *ctx, gcli_submit_comment_opts opts,
                              gcli_fetch_buffer *const out)
{
	char const *type = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	int rc = 0;

	e_owner = gcli_urlencode(opts.owner);
	e_repo = gcli_urlencode(opts.repo);

	switch (opts.target_type) {
	case ISSUE_COMMENT:
		type = "issues";
		break;
	case PR_COMMENT:
		type = "merge_requests";
		break;
	}

	char *post_fields = sn_asprintf(
		"{ \"body\": \""SV_FMT"\" }",
		SV_ARGS(opts.message));
	char *url = sn_asprintf(
		"%s/projects/%s%%2F%s/%s/%d/notes",
		gcli_get_apibase(ctx),
		e_owner, e_repo, type, opts.target_id);

	rc = gcli_fetch_with_method(ctx, "POST", url, post_fields, NULL, out);

	free(post_fields);
	free(e_owner);
	free(e_repo);
	free(url);

	return rc;
}

int
gitlab_get_mr_comments(gcli_ctx *ctx, char const *owner, char const *repo,
                       gcli_id const mr, gcli_comment_list *const out)
{
	char *e_owner = gcli_urlencode(owner);
	char *e_repo = gcli_urlencode(repo);

	gcli_fetch_list_ctx fl = {
			.listp = &out->comments,
			.sizep = &out->comments_size,
			.parse = (parsefn)parse_gitlab_comments,
			.max = -1,
	};

	char *url = sn_asprintf(
		"%s/projects/%s%%2F%s/merge_requests/%"PRIid"/notes",
		gcli_get_apibase(ctx),
		e_owner, e_repo, mr);

	free(e_owner);
	free(e_repo);

	return gcli_fetch_list(ctx, url, &fl);
}

int
gitlab_get_issue_comments(gcli_ctx *ctx, char const *owner, char const *repo,
                          gcli_id const issue, gcli_comment_list *const out)
{
	char *e_owner = gcli_urlencode(owner);
	char *e_repo = gcli_urlencode(repo);

	gcli_fetch_list_ctx fl = {
		.listp = &out->comments,
		.sizep = &out->comments_size,
		.parse = (parsefn)parse_gitlab_comments,
		.max = -1,
	};

	char *url = sn_asprintf(
		"%s/projects/%s%%2F%s/issues/%"PRIid"/notes",
		gcli_get_apibase(ctx),
		e_owner, e_repo, issue);
	free(e_owner);
	free(e_repo);

	return gcli_fetch_list(ctx, url, &fl);
}
