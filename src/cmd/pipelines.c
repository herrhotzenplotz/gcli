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
#include <gcli/cmd/table.h>

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
	fprintf(stderr, "  -o owner                 The repository owner\n");
	fprintf(stderr, "  -r repo                  The repository name\n");
	fprintf(stderr, "  -p pipeline              Fetch jobs of the given pipeline\n");
	fprintf(stderr, "  -j job                   Run actions for the given job\n");
	fprintf(stderr, "  -n number                Number of issues to fetch (-1 = everything)\n");
	fprintf(stderr, "ACTIONS:\n");
	fprintf(stderr, "  status                   Display status information\n");
	fprintf(stderr, "  artifacts [-o filename]  Download a zip archive of the artifacts of the given job\n");
	fprintf(stderr, "                           (default output filename: artifacts.zip)\n");
	fprintf(stderr, "  log                      Display job log\n");
	fprintf(stderr, "  cancel                   Cancel the job\n");
	fprintf(stderr, "  retry                    Retry the given job\n");
	fprintf(stderr, "\n");
	version();
	copyright();
}

void
gitlab_print_pipelines(gitlab_pipeline_list const *const list)
{
	gcli_tbl table;
	gcli_tblcoldef cols[] = {
		{ .name = "ID",      .type = GCLI_TBLCOLTYPE_INT,    .flags = GCLI_TBLCOL_JUSTIFYR },
		{ .name = "STATUS",  .type = GCLI_TBLCOLTYPE_STRING, .flags = GCLI_TBLCOL_STATECOLOURED },
		{ .name = "CREATED", .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "UPDATED", .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "REF",     .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
	};

	if (!list->pipelines_size) {
		printf("No pipelines\n");
		return;
	}

	table = gcli_tbl_begin(cols, ARRAY_SIZE(cols));
	if (!table)
		errx(1, "error: could not init table");

	for (size_t i = 0; i < list->pipelines_size; ++i) {
		gcli_tbl_add_row(table,
		                 (int)(list->pipelines[i].id),
		                 list->pipelines[i].status,
		                 list->pipelines[i].created_at,
		                 list->pipelines[i].updated_at,
		                 list->pipelines[i].ref);
	}

	gcli_tbl_end(table);
}

int
gitlab_pipelines(char const *owner, char const *repo, int const count)
{
	gitlab_pipeline_list pipelines = {0};
	int rc = 0;

	rc = gitlab_get_pipelines(g_clictx, owner, repo, count, &pipelines);
	if (rc < 0)
		return rc;

	gitlab_print_pipelines(&pipelines);
	gitlab_free_pipelines(&pipelines);

	return rc;
}

void
gitlab_print_jobs(gitlab_job_list const *const list)
{
	gcli_tbl table;
	gcli_tblcoldef cols[] = {
		{ .name = "ID",         .type = GCLI_TBLCOLTYPE_LONG,   .flags = GCLI_TBLCOL_JUSTIFYR },
		{ .name = "NAME",       .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "STATUS",     .type = GCLI_TBLCOLTYPE_STRING, .flags = GCLI_TBLCOL_STATECOLOURED },
		{ .name = "STARTED",    .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "FINISHED",   .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "RUNNERDESC", .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "REF",        .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
	};

	if (!list->jobs_size) {
		printf("No jobs\n");
		return;
	}

	table = gcli_tbl_begin(cols, ARRAY_SIZE(cols));
	if (!table)
		errx(1, "error: could not initialize table");

	for (size_t i = 0; i < list->jobs_size; ++i) {
		gcli_tbl_add_row(table,
		                 list->jobs[i].id,
		                 list->jobs[i].name,
		                 list->jobs[i].status,
		                 list->jobs[i].started_at,
		                 list->jobs[i].finished_at,
		                 list->jobs[i].runner_description,
		                 list->jobs[i].ref);
	}

	gcli_tbl_end(table);
}

int
gitlab_pipeline_jobs(char const *owner, char const *repo,
                     long const id, int const count)
{
	gitlab_job_list jobs = {0};
	int rc = 0;

	rc = gitlab_get_pipeline_jobs(g_clictx, owner, repo, id, count, &jobs);
	if (rc < 0)
		return rc;

	gitlab_print_jobs(&jobs);
	gitlab_free_jobs(&jobs);

	return rc;
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
	if (gcli_config_get_forge_type(g_clictx) != GCLI_FORGE_GITLAB)
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

		if (gitlab_pipeline_jobs(owner, repo, pid, count) < 0)
			errx(1, "error: failed to get pipeline jobs: %s",
			     gcli_get_error(g_clictx));
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

		if (gitlab_pipelines(owner, repo, count) < 0)
			errx(1, "error: failed to get pipelines: %s",
			     gcli_get_error(g_clictx));

		return EXIT_SUCCESS;
	}

	/* At this point jid contains a (hopefully) valid job id */

	/* Definition of the action list */
	struct {
		char const *name;                               /* Name on the cli */
		int (*fn)(gcli_ctx *ctx, char const *, char const *, long);   /* Function to be invoked for this action */
	} job_actions[] = {
		{ .name = "log",    .fn = gitlab_job_get_log },
		{ .name = "status", .fn = gitlab_job_status  },
		{ .name = "cancel", .fn = gitlab_job_cancel  },
		{ .name = "retry",  .fn = gitlab_job_retry   },
	};

	/* Check if the user missed out on supplying actions */
	if (argc == 0) {
		fprintf(stderr, "error: no actions supplied\n");
		usage();
		exit(EXIT_FAILURE);
	}

next_action:
	while (argc) {
		char const *action = shift(&argc, &argv);

		/* Handle the artifacts action separately because it allows a
		 * -o flag. No other action supports flags. */
		if (strcmp(action, "artifacts") == 0) {
			char const *outfile = "artifacts.zip";
			if (argc && strcmp(argv[0], "-o") == 0) {
				if (argc < 2)
					errx(1, "error: -o is missing the output filename");
				outfile = argv[1];
				argc -= 2;
				argv += 2;
			}
			if (gitlab_job_download_artifacts(g_clictx, owner, repo, jid, outfile) < 0)
				errx(1, "error: failed to download file");
			goto next_action;
		}

		/* Find the action and invoke it */
		for (size_t i = 0; i < ARRAY_SIZE(job_actions); ++i) {
			if (strcmp(action, job_actions[i].name) == 0) {
				if (job_actions[i].fn(g_clictx, owner, repo, jid) < 0)
					errx(1, "error: failed to perform action '%s'", action);
				goto next_action;
			}
		}

		fprintf(stderr, "error: unknown action '%s'\n", action);
		usage();
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
