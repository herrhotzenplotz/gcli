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

#include <gcli/comments.h>
#include <gcli/config.h>
#include <gcli/editor.h>
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

static void
comment_init(gcli_ctx *ctx, FILE *f, void *_data)
{
	gcli_submit_comment_opts *info = _data;
	const char *target_type = NULL;

	switch (info->target_type) {
	case ISSUE_COMMENT:
		target_type = "issue";
		break;
	case PR_COMMENT: {
		switch (gcli_config_get_forge_type(ctx)) {
		case GCLI_FORGE_GITEA:
		case GCLI_FORGE_GITHUB:
			target_type = "Pull Request";
			break;
		case GCLI_FORGE_GITLAB:
			target_type = "Merge Request";
			break;
		}
	} break;
	}

	fprintf(
		f,
		"! Enter your comment above, save and exit.\n"
		"! All lines with a leading '!' are discarded and will not\n"
		"! appear in your comment.\n"
		"! COMMENT IN : %s/%s %s #%d\n",
		info->owner, info->repo, target_type, info->target_id);
}

static sn_sv
gcli_comment_get_message(gcli_ctx *ctx, gcli_submit_comment_opts *info)
{
	return gcli_editor_get_user_message(ctx, comment_init, info);
}

int
gcli_comment_submit(gcli_ctx *ctx, gcli_submit_comment_opts opts)
{
	sn_sv const message = gcli_comment_get_message(ctx, &opts);
	opts.message = gcli_json_escape(message);
	int rc = 0;

	fprintf(
		stdout,
		"You will be commenting the following in %s/%s #%d:\n"SV_FMT"\n",
		opts.owner, opts.repo, opts.target_id, SV_ARGS(message));

	if (!opts.always_yes) {
		if (!sn_yesno("Is this okay?"))
			errx(1, "Aborted by user");
	}

	rc = gcli_forge(ctx)->perform_submit_comment(ctx, opts, NULL);

	free(message.data);
	free(opts.message.data);

	return rc;
}
