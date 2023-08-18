/*
 * Copyright 2022 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <config.h>

#include <gcli/cmd/cmd.h>
#include <gcli/cmd/colour.h>
#include <gcli/cmd/comment.h>

#include <gcli/comments.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

static void
usage(void)
{
	fprintf(stderr, "usage: gcli comment [-o owner -r repo] [-p pr | -i issue] [-y]\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "  -o owner        The repository owner\n");
	fprintf(stderr, "  -r repo         The repository name\n");
	fprintf(stderr, "  -p pr           PR id to comment under\n");
	fprintf(stderr, "  -i issue        issue id to comment under\n");
	fprintf(stderr, "  -y              Do not ask for confirmation\n");
	version();
	copyright();
}

int
gcli_issue_comments(char const *owner, char const *repo, int const issue)
{
	gcli_comment_list list = {0};
	int rc = 0;

	rc = gcli_get_issue_comments(g_clictx, owner, repo, issue, &list);
	if (rc < 0)
		return rc;

	gcli_print_comment_list(&list);
	gcli_comment_list_free(&list);

	return rc;
}

int
gcli_pull_comments(char const *owner, char const *repo, int const pull)
{
	gcli_comment_list list = {0};
	int rc = 0;

	rc = gcli_get_pull_comments(g_clictx, owner, repo, pull, &list);
	if (rc < 0)
		return rc;

	gcli_print_comment_list(&list);
	gcli_comment_list_free(&list);

	return rc;
}

void
gcli_print_comment_list(gcli_comment_list const *const list)
{
	for (size_t i = 0; i < list->comments_size; ++i) {
		printf("AUTHOR : %s%s%s\n"
		       "DATE   : %s\n",
		       gcli_setbold(), list->comments[i].author, gcli_resetbold(),
		       list->comments[i].date);
		pretty_print(list->comments[i].body, 9, 80, stdout);
		putchar('\n');
	}
}

int
subcommand_comment(int argc, char *argv[])
{
	int ch, target_id = -1, rc = 0;
	char const *repo = NULL, *owner = NULL;
	bool always_yes = false;
	enum comment_target_type target_type;

	struct option const options[] = {
		{ .name    = "yes",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 'y' },
		{ .name    = "repo",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'r' },
		{ .name    = "owner",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'o' },
		{ .name    = "issue",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'i' },
		{ .name    = "pull",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'p' },
		{0},
	};

	while ((ch = getopt_long(argc, argv, "yr:o:i:p:", options, NULL)) != -1) {
		switch (ch) {
		case 'r':
			repo = optarg;
			break;
		case 'o':
			owner = optarg;
			break;
		case 'p':
			target_type = PR_COMMENT;
			goto parse_target_id;
		case 'i':
			target_type = ISSUE_COMMENT;
		parse_target_id: {
				char *endptr;
				target_id = strtoul(optarg, &endptr, 10);
				if (endptr != optarg + strlen(optarg))
					err(1, "error: Cannot parse issue/PR number");
			} break;
		case 'y':
			always_yes = true;
			break;
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	check_owner_and_repo(&owner, &repo);

	if (target_id < 0) {
		fprintf(stderr, "error: missing issue/PR number (use -i/-p)\n");
		usage();
		return EXIT_FAILURE;
	}

	rc = gcli_comment_submit(g_clictx, (gcli_submit_comment_opts) {
			.owner       = owner,
			.repo        = repo,
			.target_type = target_type,
			.target_id   = target_id,
			.always_yes  = always_yes });

	if (rc < 0)
		errx(1, "error: failed to submit comment");

	return EXIT_SUCCESS;
}
