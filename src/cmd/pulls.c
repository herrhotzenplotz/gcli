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
#include <gcli/gitconfig.h>
#include <gcli/pulls.h>
#include <gcli/review.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <stdlib.h>

static void
usage(void)
{
	fprintf(stderr, "usage: gcli pulls create [-o owner -r repo] [-f from]\n");
	fprintf(stderr, "                         [-t to] [-d] [-l label]\n");
	fprintf(stderr, "       gcli pulls [-o owner -r repo] [-a] [-n number] [-s]\n");
	fprintf(stderr, "       gcli pulls [-o owner -r repo] -p pull actions...\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "  -o owner        The repository owner\n");
	fprintf(stderr, "  -r repo         The repository name\n");
	fprintf(stderr, "  -a              Fetch everything including closed and merged PRs\n");
	fprintf(stderr, "  -d              Mark newly created PR as a draft\n");
	fprintf(stderr, "  -f owner:branch Specify the owner and branch of the fork that is the head of a PR.\n");
	fprintf(stderr, "  -l label        Add the given label when creating the PR\n");
	fprintf(stderr, "  -n number       Number of PRs to fetch (-1 = everything)\n");
	fprintf(stderr, "  -p id           ID of PR to perform actions on\n");
	fprintf(stderr, "  -s              Print (sort) in reverse order\n");
	fprintf(stderr, "  -t branch       Specify target branch of the PR\n");
	fprintf(stderr, "  -y              Do not ask for confirmation.\n");
	fprintf(stderr, "ACTIONS:\n");
	fprintf(stderr, "  summary|status  Display status information\n");
	fprintf(stderr, "  comments        Display comments\n");
	fprintf(stderr, "  checks          Display CI/Pipeline status information about the PR\n");
	fprintf(stderr, "  merge [-s]      Merge the PR (-s = squash commits)\n");
	fprintf(stderr, "  close           Close the PR\n");
	fprintf(stderr, "  reopen          Reopen a closed PR\n");
	fprintf(stderr, "  labels ...      Add or remove labels:\n");
	fprintf(stderr, "                     --add <name>\n");
	fprintf(stderr, "                     --remove <name>\n");
	fprintf(stderr, "  diff            Display changes as diff\n");
	fprintf(stderr, "  reviews         Display reviews\n");
	fprintf(stderr, "\n");
	version();
	copyright();
}

static sn_sv
pr_try_derive_head(void)
{
	sn_sv account = {0};
	sn_sv branch  = {0};

	if (!(account = gcli_config_get_account()).length)
		errx(1,
		     "error: Cannot derive PR head. Please specify --from or set the\n"
		     "       account in the users gcli config file.");

	if (!(branch = gcli_gitconfig_get_current_branch()).length)
		errx(1,
		     "error: Cannot derive PR head. Please specify --from or, if you\n"
		     "       are in »detached HEAD« state, checkout the branch you \n"
		     "       want to pull request.");

	return sn_sv_fmt(SV_FMT":"SV_FMT, SV_ARGS(account), SV_ARGS(branch));
}

static int
subcommand_pull_create(int argc, char *argv[])
{
	/* we'll use getopt_long here to parse the arguments */
	int                       ch;
	gcli_submit_pull_options opts   = {0};

	const struct option options[] = {
		{ .name = "from",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 'f' },
		{ .name = "to",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 't' },
		{ .name = "owner",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 'o' },
		{ .name = "repo",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 'r' },
		{ .name = "draft",
		  .has_arg = no_argument,
		  .flag = &opts.draft,
		  .val = 1   },
		{ .name = "label",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 'l' },
		{0},
	};

	while ((ch = getopt_long(argc, argv, "yf:t:do:r:l:", options, NULL)) != -1) {
		switch (ch) {
		case 'f':
			opts.from  = SV(optarg);
			break;
		case 't':
			opts.to    = SV(optarg);
			break;
		case 'd':
			opts.draft = 1;
			break;
		case 'o':
			opts.owner = SV(optarg);
			break;
		case 'r':
			opts.repo = SV(optarg);
			break;
		case 'l': /* add a label */
			opts.labels = realloc(
				opts.labels, sizeof(*opts.labels) * (opts.labels_size + 1));
			opts.labels[opts.labels_size++] = optarg;
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

	if (!opts.from.length)
		opts.from = pr_try_derive_head();

	if (!opts.to.length) {
		if (!(opts.to = gcli_config_get_base()).length)
			errx(1,
			     "error: PR base is missing. Please either specify "
			     "--to branch-name or set pr.base in .gcli.");
	}

	if (!opts.owner.length || !opts.repo.length) {
		gcli_config_get_upstream_parts(&opts.owner, &opts.repo);
		if (!opts.owner.length || !opts.repo.length)
			errx(1, "error: PR target repo is missing. Please either "
			     "specify -o owner and -r repo or set pr.upstream in .gcli.");
	}

	if (argc != 1) {
		fprintf(stderr, "error: Missing title to PR\n");
		usage();
		return EXIT_FAILURE;
	}

	opts.title = SV(argv[0]);

	gcli_pr_submit(opts);

	free(opts.labels);

	return EXIT_SUCCESS;
}

int
subcommand_pulls(int argc, char *argv[])
{
	char                   *endptr     = NULL;
	const char             *owner      = NULL;
	const char             *repo       = NULL;
	gcli_pull              *pulls      = NULL;
	int                     ch         = 0;
	int                     pr         = -1;
	int                     pulls_size = 0;
	int                     n          = 30; /* how many prs to fetch at least */
	bool                    all        = false;
	enum gcli_output_flags  flags      = 0;

	/* detect whether we wanna create a PR */
	if (argc > 1 && (strcmp(argv[1], "create") == 0)) {
		shift(&argc, &argv);
		return subcommand_pull_create(argc, argv);
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
		{ .name    = "count",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'n' },
		{ .name    = "repo",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'r' },
		{ .name    = "owner",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'o' },
		{ .name    = "pull",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'p' },
		{0},
	};

	/* Parse commandline options */
	while ((ch = getopt_long(argc, argv, "+n:o:r:p:as", options, NULL)) != -1) {
		switch (ch) {
		case 'o':
			owner = optarg;
			break;
		case 'r':
			repo = optarg;
			break;
		case 'p': {
			pr = strtoul(optarg, &endptr, 10);
			if (endptr != (optarg + strlen(optarg)))
				err(1, "error: cannot parse pr number »%s«", optarg);

			if (pr <= 0)
				errx(1, "error: pr number is out of range");
		} break;
		case 'n': {
			n = strtoul(optarg, &endptr, 10);
			if (endptr != (optarg + strlen(optarg)))
				err(1, "error: cannot parse pr count »%s«", optarg);

			if (n < -1)
				errx(1, "error: pr count is out of range");
		} break;
		case 'a': {
			all = true;
		} break;
		case 's': {
			flags |= OUTPUT_SORTED;
		} break;
		case '?':
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	check_owner_and_repo(&owner, &repo);

	/* In case no explicit PR number was specified, list all
	 * open PRs and exit */
	if (pr < 0) {
		pulls_size = gcli_get_prs(owner, repo, all, n, &pulls);
		gcli_print_pr_table(flags, pulls, pulls_size);

		gcli_pulls_free(pulls, pulls_size);
		free(pulls);

		return EXIT_SUCCESS;
	}

	/* If a PR number was given, require -a to be unset */
	if (all) {
		fprintf(stderr, "error: -a cannot be combined with operations on a PR\n");
		usage();
		return EXIT_FAILURE;
	}

	/* we have an explicit PR number, so execute all operations the
	 * user has given */
	while (argc > 0) {
		const char *operation = shift(&argc, &argv);

		if (strcmp(operation, "diff") == 0) {
			gcli_print_pr_diff(stdout, owner, repo, pr);
		} else if (strcmp(operation, "summary") == 0) {
			gcli_pr_summary(owner, repo, pr);
		} else if (strcmp(operation, "status") == 0) {
			gcli_pr_status(owner, repo, pr);
		} else if (strcmp(operation, "comments") == 0) {
			gcli_pull_comments(owner, repo, pr);
		} else if (strcmp(operation, "checks") == 0) {
			gcli_pr_checks(owner, repo, pr);
		} else if (strcmp(operation, "merge") == 0) {
			/* Check whether the user intends a squash-merge */
			if (argc > 0 && (strcmp(argv[0], "-s") == 0 || strcmp(argv[0], "--squash") == 0)) {
				--argc; ++argv;
				gcli_pr_merge(owner, repo, pr, true);
			} else {
				gcli_pr_merge(owner, repo, pr, false);
			}
		} else if (strcmp(operation, "close") == 0) {
			gcli_pr_close(owner, repo, pr);
		} else if (strcmp(operation, "reopen") == 0) {
			gcli_pr_reopen(owner, repo, pr);
		} else if (strcmp(operation, "reviews") == 0) {
			/* list reviews */
			gcli_pr_review *reviews      = NULL;
			size_t          reviews_size = gcli_review_get_reviews(
				owner, repo, pr, &reviews);
			gcli_review_print_review_table(reviews, reviews_size);
			gcli_review_reviews_free(reviews, reviews_size);
		} else if (strcmp("labels", operation) == 0) {
			const char **add_labels         = NULL;
			size_t       add_labels_size    = 0;
			const char **remove_labels      = NULL;
			size_t       remove_labels_size = 0;

			if (argc == 0) {
				fprintf(stderr, "error: expected label operations\n");
				usage();
				return EXIT_FAILURE;
			}

			parse_labels_options(&argc, &argv,
			                     &add_labels,    &add_labels_size,
			                     &remove_labels, &remove_labels_size);

			/* actually go about deleting and adding the labels */
			if (add_labels_size)
				gcli_pr_add_labels(
					owner, repo, pr, add_labels, add_labels_size);
			if (remove_labels_size)
				gcli_pr_remove_labels(
					owner, repo, pr, remove_labels, remove_labels_size);

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
