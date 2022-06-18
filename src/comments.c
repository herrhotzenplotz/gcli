/*
 * Copyright 2021 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <ghcli/color.h>
#include <ghcli/comments.h>
#include <ghcli/config.h>
#include <ghcli/editor.h>
#include <ghcli/forges.h>
#include <ghcli/github/comments.h>
#include <ghcli/json_util.h>
#include <sn/sn.h>

static void
ghcli_issue_comment_free(ghcli_comment *it)
{
	free((void *)it->author);
	free((void *)it->date);
	free((void *)it->body);
}

void
ghcli_print_comment_list(
	ghcli_comment *comments,
	size_t comments_size)
{
	for (size_t i = 0; i < comments_size; ++i) {
		printf("AUTHOR : %s%s%s\n"
			   "DATE   : %s\n",
			   ghcli_setbold(), comments[i].author, ghcli_resetbold(),
			   comments[i].date);
		pretty_print(comments[i].body, 9, 80, stdout);
		putchar('\n');
	}
}

void
ghcli_issue_comments(
	const char *owner,
	const char *repo,
	int         issue)
{
	ghcli_comment *comments = NULL;
	int            n        = -1;

	n = ghcli_forge()->get_issue_comments(owner, repo, issue, &comments);
	ghcli_print_comment_list(comments, (size_t)n);

	for (int i = 0; i < n; ++i)
		ghcli_issue_comment_free(&comments[i]);

	free(comments);
}

void
ghcli_pull_comments(
	const char *owner,
	const char *repo,
	int         issue)
{
	ghcli_comment *comments = NULL;
	int            n        = -1;

	n = ghcli_forge()->get_pull_comments(owner, repo, issue, &comments);
	ghcli_print_comment_list(comments, (size_t)n);

	for (int i = 0; i < n; ++i)
		ghcli_issue_comment_free(&comments[i]);

	free(comments);
}

static void
comment_init(FILE *f, void *_data)
{
	ghcli_submit_comment_opts *info        = _data;
	const char                *target_type = NULL;

	switch (info->target_type) {
	case ISSUE_COMMENT:
		target_type = "issue";
		break;
	case PR_COMMENT: {
		switch (ghcli_config_get_forge_type()) {
		case GHCLI_FORGE_GITEA:
		case GHCLI_FORGE_GITHUB:
			target_type = "Pull Request";
			break;
		case GHCLI_FORGE_GITLAB:
			target_type = "Merge Request";
			break;
		}
	} break;
	}

	fprintf(
		f,
		"# Enter your comment below, save and exit.\n"
		"# All lines with a leading '#' are discarded and will not\n"
		"# appear in your comment.\n"
		"# COMMENT IN : %s/%s %s #%d\n",
		info->owner, info->repo, target_type, info->target_id);
}

static sn_sv
ghcli_comment_get_message(ghcli_submit_comment_opts *info)
{
	return ghcli_editor_get_user_message(comment_init, info);
}

void
ghcli_comment_submit(ghcli_submit_comment_opts opts)
{
	ghcli_fetch_buffer buffer = {0};
	sn_sv message = ghcli_comment_get_message(&opts);
	opts.message  = ghcli_json_escape(message);

	fprintf(
		stdout,
		"You will be commenting the following in %s/%s #%d:\n"SV_FMT"\n",
		opts.owner, opts.repo, opts.target_id, SV_ARGS(message));

	if (!opts.always_yes) {
		if (!sn_yesno("Is this okay?"))
			errx(1, "Aborted by user");
	}

	ghcli_forge()->perform_submit_comment(opts, &buffer);
	ghcli_print_html_url(buffer);

	free(buffer.data);
	free(message.data);
	free(opts.message.data);
}
