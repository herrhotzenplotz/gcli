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

#include <gcli/cmd/colour.h>
#include <gcli/cmd/editor.h>

#include <gcli/comments.h>
#include <gcli/forges.h>
#include <gcli/github/comments.h>
#include <gcli/json_util.h>
#include <sn/sn.h>

static void
gcli_issue_comment_free(gcli_comment *const it)
{
	free((void *)it->author);
	free((void *)it->date);
	free((void *)it->body);
}

void
gcli_comment_list_free(gcli_comment_list *list)
{
	for (size_t i = 0; i < list->comments_size; ++i)
		gcli_issue_comment_free(&list->comments[i]);

	free(list->comments);
	list->comments = NULL;
	list->comments_size = 0;
}

int
gcli_get_issue_comments(gcli_ctx *ctx, char const *owner, char const *repo,
                        int const issue, gcli_comment_list *out)
{
	return gcli_forge(ctx)->get_issue_comments(ctx, owner, repo, issue, out);
}

int
gcli_get_pull_comments(gcli_ctx *ctx, char const *owner, char const *repo,
                       int const pull, gcli_comment_list *out)
{
	return gcli_forge(ctx)->get_pull_comments(ctx, owner, repo, pull, out);
}

int
gcli_comment_submit(gcli_ctx *ctx, gcli_submit_comment_opts opts)
{
	return gcli_forge(ctx)->perform_submit_comment(ctx, opts, NULL);
}
