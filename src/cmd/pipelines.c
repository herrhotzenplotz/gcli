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
#include <gcli/config.h>
#include <gcli/forges.h>
#include <gcli/gitlab/pipelines.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <stdlib.h>

static void
usage(void)
{
	fprintf(stderr, "usage: gcli pipelines [-o owner -r repo] [-n number]\n");
	fprintf(stderr, "       gcli pipelines [-o owner -r repo] -p pipeline [-n number]\n");
	fprintf(stderr, "       gcli pipelines [-o owner -r repo] -j job [-n number] actions...\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "  -o owner        The repository owner\n");
	fprintf(stderr, "  -r repo         The repository name\n");
	fprintf(stderr, "  -p pipeline     Fetch jobs of the given pipeline\n");
	fprintf(stderr, "  -j job          Run actions for the given job\n");
	fprintf(stderr, "  -n number       Number of issues to fetch (-1 = everything)\n");
	fprintf(stderr, "ACTIONS:\n");
	fprintf(stderr, "  status          Display status information\n");
	fprintf(stderr, "  log             Display job log\n");
	fprintf(stderr, "  cancel          Cancel the job\n");
	fprintf(stderr, "  retry           Retry the given job\n");
	fprintf(stderr, "\n");
	version();
	copyright();
}

int
subcommand_pipelines(int argc, char *argv[])
{
	int         ch    = 0;
	char const *owner = NULL, *repo = NULL;
	int         count = 30;
	long        pid   = -1;     /* pipeline id                           */
	long        jid   = -1;     /* job id. these are mutually exclusive. */

	/* Parse options */
	const struct option options[] = {
		{.name = "repo",     .has_arg = required_argument, .flag = NULL, .val = 'r'},
		{.name = "owner",    .has_arg = required_argument, .flag = NULL, .val = 'o'},
		{.name = "count",    .has_arg = required_argument, .flag = NULL, .val = 'c'},
		{.name = "pipeline", .has_arg = required_argument, .flag = NULL, .val = 'p'},
		{.name = "job",      .has_arg = required_argument, .flag = NULL, .val = 'j'},
		{0}
	};

	while ((ch = getopt_long(argc, argv, "+n:o:r:p:j:", options, NULL)) != -1) {
		switch (ch) {
		case 'o':
			owner = optarg;
			break;
		case 'r':
			repo = optarg;
			break;
		case 'n': {
			char *endptr = NULL;
			count        = strtol(optarg, &endptr, 10);
			if (endptr != (optarg + strlen(optarg)))
				err(1, "ci: cannot parse argument to -n");
		} break;
		case 'p': {
			char *endptr = NULL;
			pid          = strtol(optarg, &endptr, 10);
			if (endptr != (optarg + strlen(optarg)))
				err(1, "ci: cannot parse argument to -p");
			if (pid < 0) {
				errx(1, "error: pipeline id must be a positive number");
			}
		} break;
		case 'j': {
			char *endptr = NULL;
			jid          = strtol(optarg, &endptr, 10);
			if (endptr != (optarg + strlen(optarg)))
				err(1, "ci: cannot parse argument to -j");
			if (jid < 0) {
				errx(1, "error: job id must be a positive number");
			}
		} break;
		case '?':
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	if (pid > 0 && jid > 0) {
		fprintf(stderr, "error: -p and -j are mutually exclusive\n");
		usage();
		return EXIT_FAILURE;
	}

	check_owner_and_repo(&owner, &repo);

	/* Make sure we are actually talking about a gitlab remote because
	 * we might be incorrectly inferring it */
	if (gcli_config_get_forge_type() != GCLI_FORGE_GITLAB)
		errx(1, "error: The pipelines subcommand only works for GitLab. "
		     "Use gcli -t gitlab ... to force a GitLab remote.");

	/* If the user specified a pipeline id, print the jobs of that
	 * given pipeline */
	if (pid >= 0) {
		/* Make sure we are interpreting things correctly */
		if (argc != 0) {
			fprintf(stderr, "error: stray arguments\n");
			usage();
			return EXIT_FAILURE;
		}

		gitlab_pipeline_jobs(owner, repo, pid, count);
		return EXIT_SUCCESS;
	}

	/* if the user didn't specify the -j option to list jobs, list the
	 * pipelines instead */
	if (jid < 0) {
		/* Make sure we are interpreting things correctly */
		if (argc != 0) {
			fprintf(stderr, "error: stray arguments\n");
			usage();
			return EXIT_FAILURE;
		}

		gitlab_pipelines(owner, repo, count);
		return EXIT_SUCCESS;
	}

	/* At this point jid contains a (hopefully) valid job id */

	/* Definition of the action list */
	struct {
		char const *name;                               /* Name on the cli */
		void (*fn)(char const *, char const *, long);   /* Function to be invoked for this action */
	} job_actions[] = {
		{ .name = "log",    .fn = gitlab_job_get_log },
		{ .name = "status", .fn = gitlab_job_status  },
		{ .name = "cancel", .fn = gitlab_job_cancel  },
		{ .name = "retry",  .fn = gitlab_job_retry   },
	};

next_action:
	while (argc) {
		char const *action = shift(&argc, &argv);

		/* Find the action and invoke it */
		for (size_t i = 0; i < ARRAY_SIZE(job_actions); ++i) {
			if (strcmp(action, job_actions[i].name) == 0) {
				job_actions[i].fn(owner, repo, jid);
				goto next_action;
			}
		}

		fprintf(stderr, "error: unknown action '%s'\n", action);
		usage();
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
