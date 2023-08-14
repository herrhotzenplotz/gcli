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
#include <gcli/repos.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <stdlib.h>

static void
usage(void)
{
	fprintf(stderr, "usage: gcli repos create -r repo [-d description] [-p]\n");
	fprintf(stderr, "       gcli repos [-o owner -r repo] [-n number] [-s] [-y] [delete]\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "  -o owner        The repository owner\n");
	fprintf(stderr, "  -r repo         The repository name\n");
	fprintf(stderr, "  -n number       Number of repos to fetch (-1 = everything)\n");
	fprintf(stderr, "  -p              Make the repo private\n");
	fprintf(stderr, "  -s              Print (sort) in reverse order\n");
	fprintf(stderr, "  -y              Do not ask for confirmation\n");
	fprintf(stderr, "\n");
	version();
	copyright();
}

static int
subcommand_repos_create(int argc, char *argv[])
{
	int ch;
	gcli_repo_create_options create_options = {0};
	gcli_repo repo = {0};

	const struct option options[] = {
		{ .name    = "repo",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'r' },
		{ .name    = "private",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 'p' },
		{ .name    = "description",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'd' },
		{0},
	};

	while ((ch = getopt_long(argc, argv, "r:d:p", options, NULL)) != -1) {
		switch (ch) {
		case 'r':
			create_options.name = SV(optarg);
			break;
		case 'd':
			create_options.description = SV(optarg);
			break;
		case 'p':
			create_options.private = true;
			break;
		case '?':
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	if (sn_sv_null(create_options.name)) {
		fprintf(stderr,
		        "name cannot be empty. please set a repository "
		        "name with -r/--name\n");
		usage();
		return EXIT_FAILURE;
	}

	if (gcli_repo_create(&create_options, &repo) < 0)
		errx(1, "error: failed to create repository");

	gcli_repo_print(&repo);
	gcli_repo_free(&repo);

	return EXIT_SUCCESS;
}

int
subcommand_repos(int argc, char *argv[])
{
	int                     ch, n = 30;
	char const             *owner      = NULL;
	char const             *repo       = NULL;
	gcli_repo_list          repos      = {0};
	bool                    always_yes = false;
	enum gcli_output_flags  flags      = 0;

	/* detect whether we wanna create a repo */
	if (argc > 1 && (strcmp(argv[1], "create") == 0)) {
		shift(&argc, &argv);
		return subcommand_repos_create(argc, argv);
	}

	struct option const options[] = {
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
		{ .name    = "yes",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 'y' },
		{ .name    = "sorted",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 's' },
		{0},
	};

	while ((ch = getopt_long(argc, argv, "n:o:r:ys", options, NULL)) != -1) {
		switch (ch) {
		case 'o':
			owner = optarg;
			break;
		case 'r':
			repo = optarg;
			break;
		case 'y':
			always_yes = true;
			break;
		case 's':
			flags |= OUTPUT_SORTED;
			break;
		case 'n': {
			char *endptr = NULL;
			n = strtol(optarg, &endptr, 10);
			if (endptr != (optarg + strlen(optarg)))
				err(1, "repos: cannot parse repo count");

			if (n == 0)
				errx(1, "error: number of repos must not be zero");
		} break;
		case '?':
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	/* List repos of the owner */
	if (argc == 0) {
		int rc = 0;

		if (repo) {
			fprintf(stderr, "error: repos: no actions specified\n");
			usage();
			return EXIT_FAILURE;
		}

		if (!owner)
		    rc = gcli_get_own_repos(n, &repos);
		else
			rc = gcli_get_repos(owner, n, &repos);

		if (rc < 0)
			errx(1, "error: failed to fetch repos");

		gcli_print_repos_table(flags, &repos, n);
		gcli_repos_free(&repos);
	} else {
		check_owner_and_repo(&owner, &repo);

		for (size_t i = 0; i < (size_t)argc; ++i) {
			char const *action = argv[i];

			if (strcmp(action, "delete") == 0) {
				delete_repo(always_yes, owner, repo);
			} else {
				fprintf(stderr, "error: repos: unknown action '%s'\n", action);
				usage();
				return EXIT_FAILURE;
			}
		}
	}

	return EXIT_SUCCESS;
}
