/*
 * Copyright 2023 Nico Sonack <nsonack@herrhotzenplotz.de>
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
#include <gcli/milestones.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

static void
usage(void)
{
	fprintf(stderr, "usage: gcli milestones [-o owner -r repo]\n");
	fprintf(stderr, "       gcli milestones [-o owner -r repo] -i milestone action...\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "  -o owner        The repository owner\n");
	fprintf(stderr, "  -r repo         The repository name\n");
	fprintf(stderr, "  -i milestone    Run actions for the given milestone id\n");
	fprintf(stderr, "ACTIONS:\n");
	fprintf(stderr, "  status          Display general status information about the milestone\n");
	fprintf(stderr, "  issues          List issues associated with the milestone\n");
	fprintf(stderr, "  delete          Delete this milestone\n");
	fprintf(stderr, "\n");
	version();
	copyright();
}

static int handle_milestone_actions(int argc, char *argv[],
                                    char const *const owner,
                                    char const *const repo,
                                    int const milestone_id);


static int
subcommand_milestone_create(int argc, char *argv[])
{
	int ch;
	struct gcli_milestone_create_args args = {0};
	struct option const options[] = {
		{ .name = "owner",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 'o' },
		{ .name = "repo",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 'r' },
		{ .name = "title",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 't' },
		{ .name  = "description",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 'd' },
	};

	/* Read in options */
	while ((ch = getopt_long(argc, argv, "+o:r:t:d:", options, NULL)) != -1) {
		switch (ch) {
		case 'o':
			args.owner = optarg;
			break;
		case 'r':
			args.repo = optarg;
			break;
		case 't':
			args.title = optarg;
			break;
		case 'd':
			args.description = optarg;
			break;
		default:
			usage();
			return 1;
		}
	}

	argc -= optind;
	argv += optind;

	/* sanity check argumets */
	if (argc)
		errx(1, "error: stray arguments");

	/* make sure both are set or deduce them */
	check_owner_and_repo(&args.owner, &args.repo);

	/* enforce the user to at least provide a title */
	if (!args.title)
		errx(1, "error: missing milestone title");

	/* actually create the milestone */
	if (gcli_create_milestone(&args) < 0)
		errx(1, "error: could not create milestone");

	return 0;
}

int
subcommand_milestones(int argc, char *argv[])
{
	int ch, rc, max = 30, milestone_id = -1;
	char const *repo, *owner;
	struct option const options[] = {
		{ .name = "owner",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 'o' },
		{ .name = "repo",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 'r' },
		{ .name = "count",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 'n' },
		{ .name = "id",
		  .has_arg = required_argument,
		  .flag = NULL,
		  .val = 'i' },
	};

	/* detect whether we wanna create a milestone */
	if (argc > 1 && (strcmp(argv[1], "create") == 0)) {
		shift(&argc, &argv);
		return subcommand_milestone_create(argc, argv);
	}

	/* Proceed with fetching information */
	repo = NULL;
	owner = NULL;

	while ((ch = getopt_long(argc, argv, "+o:r:n:i:", options, NULL)) != -1) {
		switch (ch) {
		case 'o': {
			owner = optarg;
		} break;
		case 'r': {
			repo = optarg;
		} break;
		case 'n': {
			char *endptr;
			max = strtol(optarg, &endptr, 10);
			if (endptr != optarg + strlen(optarg))
				errx(1, "error: cannot parse milestone count");
		} break;
		case 'i': {
			char *endptr;
			milestone_id = strtol(optarg, &endptr, 10);
			if (endptr != optarg + strlen(optarg))
				errx(1, "error: cannot parse milestone id");

			if (milestone_id < 0)
				errx(1, "error: milestone id must not be negative");
		} break;
		default: {
			usage();
			return 1;
		}
		}
	}

	argc -= optind;
	argv += optind;

	check_owner_and_repo(&owner, &repo);

	if (milestone_id < 0) {
		gcli_milestone_list list = {0};

		rc = gcli_get_milestones(owner, repo, max, &list);
		if (rc < 0)
			errx(1, "error: cannot get list of milestones");

		gcli_print_milestones(&list, max);
		gcli_free_milestones(&list);

		return 0;
	}

	return handle_milestone_actions(argc, argv, owner, repo, milestone_id);
}

static void
ensure_milestone(char const *const owner,
                 char const *const repo,
                 int const milestone_id,
                 int *const fetched_milestone,
                 gcli_milestone *const milestone)
{
	int rc;

	if (*fetched_milestone)
		return;

	rc = gcli_get_milestone(owner, repo, milestone_id, milestone);
	if (rc < 0)
		errx(1, "error: could not get milestone %d", milestone_id);

	*fetched_milestone = 1;
}

static int
handle_milestone_actions(int argc, char *argv[],
                         char const *const owner,
                         char const *const repo,
                         int const milestone_id)
{
	gcli_milestone milestone = {0};
	int fetched_milestone = 0;

	/* Iterate over all the actions */
	while (argc) {
		/* Read in action */
		char const *action = shift(&argc, &argv);

		/* Dispatch */
		if (strcmp(action, "status") == 0) {

			/* Make sure we have the milestone data */
			ensure_milestone(owner, repo, milestone_id,
			                 &fetched_milestone, &milestone);

			/* Print meta */
			gcli_print_milestone(&milestone);
		} else if (strcmp(action, "issues") == 0) {

			gcli_issue_list issues = {0};

			/* Fetch list of issues associated with milestone */
			gcli_milestone_get_issues(owner, repo, milestone_id, &issues);

			/* Print them as a table */
			gcli_print_issues_table(0, &issues, -1);

			/* Cleanup */
			gcli_issues_free(&issues);

		} else if (strcmp(action, "delete") == 0) {

			/* Delete the milestone */
			gcli_delete_milestone(owner, repo, milestone_id);

		} else {

			/* We don't know of the action - maybe a syntax error or
			 * trailing garbage. Error out in this case. */
			fprintf(stderr, "error: unknown action %s\n", action);
			usage();
			return EXIT_FAILURE;
		}


		/* Print a blank line if we are not at the end */
		if (argc)
			putchar('\n');
	}

	/* Cleanup the milestone if we ever fetched it */
	if (fetched_milestone)
		gcli_free_milestone(&milestone);

	return EXIT_SUCCESS;
}
