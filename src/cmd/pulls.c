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
	fprintf(stderr, "                         [-t to] [-d] [-l label] pull-request-title\n");
	fprintf(stderr, "       gcli pulls [-o owner -r repo] [-a] [-A author ][-n number] [-s]\n");
	fprintf(stderr, "       gcli pulls [-o owner -r repo] -i pull-id actions...\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "  -o owner        The repository owner\n");
	fprintf(stderr, "  -r repo         The repository name\n");
	fprintf(stderr, "  -a              Fetch everything including closed and merged PRs\n");
	fprintf(stderr, "  -A author       Filter pull requests by the given author\n");
	fprintf(stderr, "  -d              Mark newly created PR as a draft\n");
	fprintf(stderr, "  -f owner:branch Specify the owner and branch of the fork that is the head of a PR.\n");
	fprintf(stderr, "  -l label        Add the given label when creating the PR\n");
	fprintf(stderr, "  -n number       Number of PRs to fetch (-1 = everything)\n");
	fprintf(stderr, "  -i id           ID of PR to perform actions on\n");
	fprintf(stderr, "  -s              Print (sort) in reverse order\n");
	fprintf(stderr, "  -t branch       Specify target branch of the PR\n");
	fprintf(stderr, "  -y              Do not ask for confirmation.\n");
	fprintf(stderr, "ACTIONS:\n");
	fprintf(stderr, "  all             Display status, commits, op and checks of the PR\n");
	fprintf(stderr, "  op              Display original post\n");
	fprintf(stderr, "  status          Display PR metadata\n");
	fprintf(stderr, "  comments        Display comments\n");
	fprintf(stderr, "  notes           Alias for notes\n");
	fprintf(stderr, "  commits         Display commits of the PR\n");
	fprintf(stderr, "  ci              Display CI/Pipeline status information about the PR\n");
	fprintf(stderr, "  merge [-s] [-D] Merge the PR (-s = squash commits, -d = inhibit deleting source branch)\n");
	fprintf(stderr, "  milestone <id>  Assign this PR to a milestone\n");
	fprintf(stderr, "  milestone -d    Clear associated milestones from the PR\n");
	fprintf(stderr, "  close           Close the PR\n");
	fprintf(stderr, "  reopen          Reopen a closed PR\n");
	fprintf(stderr, "  labels ...      Add or remove labels:\n");
	fprintf(stderr, "                     add <name>\n");
	fprintf(stderr, "                     remove <name>\n");
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
			opts.owner = optarg;
			break;
		case 'r':
			opts.repo = optarg;
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

	check_owner_and_repo(&opts.owner, &opts.repo);

	if (argc != 1) {
		fprintf(stderr, "error: Missing title to PR\n");
		usage();
		return EXIT_FAILURE;
	}

	opts.title = SV(argv[0]);

	if (gcli_pull_submit(opts) < 0)
		errx(1, "error: failed to submit pull request");

	free(opts.labels);

	return EXIT_SUCCESS;
}

/* Forward declaration */
static int handle_pull_actions(int argc, char *argv[],
                               char const *owner,
                               char const *repo,
                               int pr);

int
subcommand_pulls(int argc, char *argv[])
{
	char *endptr = NULL;
	char const *owner = NULL;
	char const *repo = NULL;
	gcli_pull_list pulls = {0};
	int ch = 0;
	int pr = -1;
	int n = 30;                 /* how many prs to fetch at least */
	gcli_pull_fetch_details details = {0};
	enum gcli_output_flags flags = 0;

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
		{ .name    = "author",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 'A' },
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
		{ .name    = "id",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'i' },
		{0},
	};

	/* Parse commandline options */
	while ((ch = getopt_long(argc, argv, "+n:o:r:i:asA:", options, NULL)) != -1) {
		switch (ch) {
		case 'o':
			owner = optarg;
			break;
		case 'r':
			repo = optarg;
			break;
		case 'i': {
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

			if (n == 0)
				errx(1, "error: pr count must not be zero");
		} break;
		case 'a': {
			details.all = true;
		} break;
		case 'A': {
			details.author = optarg;
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
		if (gcli_get_pulls(owner, repo, &details, n, &pulls) < 0)
			errx(1, "error: could not fetch pull requests");

		gcli_print_pulls_table(flags, &pulls, n);
		gcli_pulls_free(&pulls);

		return EXIT_SUCCESS;
	}

	/* If a PR number was given, require -a to be unset */
	if (details.all || details.author) {
		fprintf(stderr, "error: -a and -A cannot be combined with operations on a PR\n");
		usage();
		return EXIT_FAILURE;
	}

	/* Hand off to actions handling */
	return handle_pull_actions(argc, argv, owner, repo, pr);
}

/** Helper routine for fetching a PR if required */
static void
ensure_pull(char const *owner, char const *repo, int pr,
            int *const fetched_pull, gcli_pull *const pull)
{
	if (*fetched_pull)
		return;

	gcli_get_pull(owner, repo, pr, pull);
	*fetched_pull = 1;
}

/** Handling routine for Pull Request related actions specified on the
 * command line. Make sure that the usage at the top is consistent
 * with the actions implemented here. */
static int
handle_pull_actions(int argc, char *argv[],
                    char const *owner, char const *repo,
                    int pr)
{
	/* For ease of handling and not making redundant calls to the API
	 * we'll fetch the summary only if a command requires it. Then
	 * we'll proceed to actually handling it. */
	int fetched_pull = 0;
	gcli_pull pull = {0};

	/* Check if the user missed out on supplying actions */
	if (argc == 0) {
		fprintf(stderr, "error: no actions supplied\n");
		usage();
		exit(EXIT_FAILURE);
	}

	/* Iterate over the argument list until the end */
	while (argc > 0) {

		/* Grab the next action from the argument list */
		const char *action = shift(&argc, &argv);

		/* Check if it is a valid one. When we find any of
		 *
		 *      all, op or status
		 *
		 * we must ensure that the summary has been fetched. */
		if (strcmp(action, "all") == 0) {

			/* First make sure we have the data ready */
			ensure_pull(owner, repo, pr, &fetched_pull, &pull);

			/* Print meta */
			gcli_pull_print_status(&pull);

			/* OP */
			puts("\nORIGINAL POST");
			gcli_pull_print_op(&pull);

			/* Commits */
			puts("\nCOMMITS");
			gcli_pull_commits(owner, repo, pr);

			/* Checks */
			puts("\nCHECKS");
			gcli_pull_checks(owner, repo, pr);

		} else if (strcmp(action, "op") == 0) {

			/* Ensure we have fetched the data */
			ensure_pull(owner, repo, pr, &fetched_pull, &pull);

			/* Print it */
			gcli_pull_print_op(&pull);

		} else if (strcmp(action, "status") == 0) {

			/* Ensure we have the data */
			ensure_pull(owner, repo, pr, &fetched_pull, &pull);

			/* Print meta information */
			gcli_pull_print_status(&pull);

		} else if (strcmp(action, "commits") == 0) {

			/* Does not require the summary */
			gcli_pull_commits(owner, repo, pr);

		} else if (strcmp(action, "diff") == 0) {
			gcli_print_pull_diff(stdout, owner, repo, pr);

		} else if (strcmp(action, "comments") == 0 ||
		           strcmp(action, "notes") == 0) {
			gcli_pull_comments(owner, repo, pr);

		} else if (strcmp(action, "ci") == 0) {
			gcli_pull_checks(owner, repo, pr);

		} else if (strcmp(action, "merge") == 0) {
			enum gcli_merge_flags flags = GCLI_PULL_MERGE_DELETEHEAD;

			/* Default behaviour */
			if (gcli_config_pr_inhibit_delete_source_branch())
			    flags = 0;

			if (argc > 0) {
				/* Check whether the user intends a squash-merge
				 * and/or wants to delete the source branch of the
				 * PR */
				if (strcmp(argv[0], "-s") == 0 ||
				    strcmp(argv[0], "--squash") == 0) {
					--argc; ++argv;
					flags |= GCLI_PULL_MERGE_SQUASH;
				} else if (strcmp(argv[0], "-D") == 0 ||
				           strcmp(argv[0], "--inhibit-delete") == 0) {
					--argc; ++argv;
					flags &= ~GCLI_PULL_MERGE_DELETEHEAD;
				}
			}

			if (gcli_pull_merge(owner, repo, pr, flags) < 0)
				errx(1, "error: failed to merge pull request");

		} else if (strcmp(action, "close") == 0) {
			if (gcli_pull_close(owner, repo, pr) < 0)
				errx(1, "error: failed to close pull request");

		} else if (strcmp(action, "reopen") == 0) {
			if (gcli_pull_reopen(owner, repo, pr) < 0)
				errx(1, "error: failed to reopen pull request");

		} else if (strcmp(action, "reviews") == 0) {
			/* list reviews */
			gcli_pr_review *reviews      = NULL;
			size_t          reviews_size = gcli_review_get_reviews(
				owner, repo, pr, &reviews);
			gcli_review_print_review_table(reviews, reviews_size);
			gcli_review_reviews_free(reviews, reviews_size);

		} else if (strcmp("labels", action) == 0) {
			const char **add_labels         = NULL;
			size_t       add_labels_size    = 0;
			const char **remove_labels      = NULL;
			size_t       remove_labels_size = 0;
			int          rc = 0;

			if (argc == 0) {
				fprintf(stderr, "error: expected label action\n");
				usage();
				return EXIT_FAILURE;
			}

			parse_labels_options(&argc, &argv,
			                     &add_labels,    &add_labels_size,
			                     &remove_labels, &remove_labels_size);

			/* actually go about deleting and adding the labels */
			if (add_labels_size) {
				rc = gcli_pull_add_labels(
					owner, repo, pr, add_labels, add_labels_size);
				if (rc < 0)
					errx(1, "failed to add labels");
			}
			if (remove_labels_size) {
				rc = gcli_pull_remove_labels(
					owner, repo, pr, remove_labels, remove_labels_size);

				if (rc < 0)
					errx(1, "failed to remove labels");
			}

			free(add_labels);
			free(remove_labels);

		} else if (strcmp("milestone", action) == 0) {
			char const *arg = shift(&argc, &argv);

			if (strcmp(arg, "-d") == 0) {
				if (gcli_pull_clear_milestone(owner, repo, pr) < 0)
					errx(1, "failed to clear milestone");

			} else {
				int milestone_id = 0;
				char *endptr;

				milestone_id = strtoul(arg, &endptr, 10);
				if (endptr != arg + strlen(arg)) {
					fprintf(stderr, "error: cannot parse milestone id »%s«\n", arg);
					return EXIT_FAILURE;
				}

				if (gcli_pull_set_milestone(owner, repo, pr, milestone_id) < 0)
					errx(1, "error: failed to set milestone");
			}
		} else {
			/* At this point we found an unknown action / stray
			 * options on the command line. Error out in this case. */

			fprintf(stderr, "error: unknown action %s\n", action);
			usage();
			return EXIT_FAILURE;

		}

		if (argc)
			putchar('\n');

	} /* Next action */

	/* Free the pull request data only when we actually fetched it */
	if (fetched_pull)
		gcli_pull_free(&pull);

	return EXIT_SUCCESS;
}
