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

#include "config.h"

#include <gcli/cmd.h>
#include <gcli/comments.h>
#include <gcli/config.h>
#include <gcli/issues.h>

#include <stdlib.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

static void
usage(void)
{
	fprintf(stderr, "usage: gcli issues create [-o owner -r repo] [-y] title...\n");
	fprintf(stderr, "       gcli issues [-o owner -r repo] [-a] [-n number] [-s]\n");
	fprintf(stderr, "       gcli issues [-o owner -r repo] -i issue actions...\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "  -o owner        The repository owner\n");
	fprintf(stderr, "  -r repo         The repository name\n");
	fprintf(stderr, "  -y              Do not ask for confirmation.\n");
	fprintf(stderr, "  -a              Fetch everything including closed issues \n");
	fprintf(stderr, "  -s              Print (sort) in reverse order\n");
	fprintf(stderr, "  -n number       Number of issues to fetch (-1 = everything)\n");
	fprintf(stderr, "  -i issue        ID of issue to perform actions on\n");
	fprintf(stderr, "ACTIONS:\n");
	fprintf(stderr, "  summary|status  Display status information\n");
	fprintf(stderr, "  comments        Display comments\n");
	fprintf(stderr, "  close           Close the issue\n");
	fprintf(stderr, "  reopen          Reopen a closed issue\n");
	fprintf(stderr, "  assign <user>   Assign the issue to the given user\n");
	fprintf(stderr, "  labels ...      Add or remove labels:\n");
	fprintf(stderr, "                     --add <name>\n");
	fprintf(stderr, "                     --remove <name>\n");
	fprintf(stderr, "\n");
	version();
	copyright();
}

static int
subcommand_issue_create(int argc, char *argv[])
{
	/* we'll use getopt_long here to parse the arguments */
	int                       ch;
	gcli_submit_issue_options opts   = {0};

	const struct option options[] = {
		{ .name    = "owner",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'o' },
		{ .name    = "repo",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'r' },
		{ .name    = "yes",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 'y' },
		{0},
	};

	while ((ch = getopt_long(argc, argv, "o:r:", options, NULL)) != -1) {
		switch (ch) {
		case 'o':
			opts.owner = SV(optarg);
			break;
		case 'r':
			opts.repo = SV(optarg);
			break;
		case 'y':
			opts.always_yes = true;
			break;
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	if (!opts.owner.length || !opts.repo.length) {
		gcli_config_get_upstream_parts(&opts.owner, &opts.repo);
		if (!opts.owner.length || !opts.repo.length)
			errx(1,
			     "error: Target repo for the issue to be created "
			     "in is missing. Please either specify '-o owner' "
			     "and '-r repo' or set pr.upstream in .gcli.");
	}

	if (argc != 1) {
		fprintf(stderr, "error: Expected one argument for issue title\n");
		usage();
		return EXIT_FAILURE;
	}

	opts.title = SV(argv[0]);

	gcli_issue_submit(opts);

	return EXIT_SUCCESS;
}

int
subcommand_issues(int argc, char *argv[])
{
	gcli_issue             *issues      = NULL;
	int                     issues_size = 0;
	char const             *owner       = NULL;
	char const             *repo        = NULL;
	char                   *endptr      = NULL;
	int                     ch          = 0;
	int                     issue       = -1;
	int                     n           = 30;
	bool                    all         = false;
	enum gcli_output_flags  flags       = 0;

	/* detect whether we wanna create an issue */
	if (argc > 1 && (strcmp(argv[1], "create") == 0)) {
		shift(&argc, &argv);
		return subcommand_issue_create(argc, argv);
	}

	const struct option options[] = {
		{ .name    = "all",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 'a' },
		{ .name    = "sorted",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 's' },
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
		{ .name    = "count",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'n'
		},
		{0},
	};

	/* parse options */
	while ((ch = getopt_long(argc, argv, "+sn:o:r:i:a", options, NULL)) != -1) {
		switch (ch) {
		case 'o':
			owner = optarg;
			break;
		case 'r':
			repo = optarg;
			break;
		case 'i': {
			issue = strtol(optarg, &endptr, 10);
			if (endptr != (optarg + strlen(optarg)))
				err(1, "error: cannot parse issue number");

			if (issue < 0)
				errx(1, "error: issue number is out of range");
		} break;
		case 'n': {
			n = strtol(optarg, &endptr, 10);
			if (endptr != (optarg + strlen(optarg)))
				err(1, "error: cannot parse issue count");

			if (n < -1)
				errx(1, "error: issue count is out of range");
		} break;
		case 'a':
			all = true;
			break;
		case 's':
			flags |= OUTPUT_SORTED;
			break;
		case '?':
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;


	check_owner_and_repo(&owner, &repo);

	/* No issue number was given, so list all open issues */
	if (issue < 0) {
		issues_size = gcli_get_issues(owner, repo, all, n, &issues);
		gcli_print_issues_table(flags, issues, issues_size);

		gcli_issues_free(issues, issues_size);
		return EXIT_SUCCESS;
	}

	/* require -a to not be set */
	if (all) {
		fprintf(stderr, "error: -a cannot be combined with operations on an issue\n");
		usage();
		return EXIT_FAILURE;
	}

	/* execute all operations on the given issue */
	while (argc > 0) {
		char const *operation = shift(&argc, &argv);

		if (strcmp("comments", operation) == 0) {
			gcli_issue_comments(owner, repo, issue);
		} else if (strcmp("summary", operation) == 0
		           || strcmp("status", operation) == 0) {
			gcli_issue_summary(owner, repo, issue);
		} else if (strcmp("close", operation) == 0) {
			gcli_issue_close(owner, repo, issue);
		} else if (strcmp("reopen", operation) == 0) {
			gcli_issue_reopen(owner, repo, issue);
		} else if (strcmp("assign", operation) == 0) {
			char const *assignee = shift(&argc, &argv);
			gcli_issue_assign(owner, repo, issue, assignee);
		} else if (strcmp("labels", operation) == 0) {
			char const **add_labels         = NULL;
			size_t       add_labels_size    = 0;
			char const **remove_labels      = NULL;
			size_t       remove_labels_size = 0;

			if (argc == 0) {
				fprintf(stderr, "error: expected label operations\n");
				usage();
				return EXIT_FAILURE;
			}

			parse_labels_options(&argc, &argv,
			                     &add_labels, &add_labels_size,
			                     &remove_labels, &remove_labels_size);

			/* actually go about deleting and adding the labels */
			if (add_labels_size)
				gcli_issue_add_labels(owner, repo, issue,
				                      add_labels, add_labels_size);
			if (remove_labels_size)
				gcli_issue_remove_labels(owner, repo, issue,
				                         remove_labels, remove_labels_size);

			free(add_labels);
			free(remove_labels);
		} else {
			fprintf(stderr, "error: unknown operation %s\n", operation);
			usage();
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}
