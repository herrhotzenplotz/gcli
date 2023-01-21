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
	fprintf(stderr, "       gcli milestones [-o owner -r repo] -i milestone\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "  -o owner        The repository owner\n");
	fprintf(stderr, "  -r repo         The repository name\n");
	fprintf(stderr, "  -i milestone    Fetch details about the given milestone ID\n");
	fprintf(stderr, "\n");
	version();
	copyright();
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
	} else {
		gcli_milestone milestone = {0};

		rc = gcli_get_milestone(owner, repo, milestone_id, &milestone);
		if (rc < 0)
			errx(1, "error: could not get milestone %d", milestone_id);

		gcli_print_milestone(&milestone);
		gcli_free_milestone(&milestone);
	}

	return 0;
}
