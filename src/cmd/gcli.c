/*
 * Copyright 2021,2022 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#ifdef HAVE_GETOPT_h
#include <getopt.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gcli/cmd/ci.h>
#include <gcli/cmd/cmd.h>
#include <gcli/cmd/comment.h>
#include <gcli/cmd/config.h>
#include <gcli/cmd/forks.h>
#include <gcli/cmd/issues.h>
#include <gcli/cmd/labels.h>
#include <gcli/cmd/milestones.h>
#include <gcli/cmd/pipelines.h>
#include <gcli/cmd/pulls.h>
#include <gcli/cmd/releases.h>
#include <gcli/cmd/repos.h>

#include <gcli/config.h>

static void usage(void);

static int
subcommand_version(int argc, char *argv[])
{
	(void) argc;
	(void) argv;

	version();
	copyright();

	return EXIT_SUCCESS;
}

static struct subcommand {
	char const *const cmd_name;
	char const *const docstring;
	int (*fn)(int, char **);
} subcommands[] = {
	{ .cmd_name = "ci",
	  .fn = subcommand_ci,
	  .docstring = "Github CI status info" },
	{ .cmd_name = "comment",
	  .fn = subcommand_comment,
	  .docstring = "Comment under issues and PRs" },
	{ .cmd_name = "config",
	  .fn = subcommand_config,
	  .docstring = "Configure forges" },
	{ .cmd_name = "forks",
	  .fn = subcommand_forks,
	  .docstring = "Create, delete and list repository forks" },
	{ .cmd_name = "gists",
	  .fn = subcommand_gists,
	  .docstring = "Create, fetch and list Github Gists" },
	{ .cmd_name = "issues",
	  .fn = subcommand_issues,
	  .docstring = "Manage issues" },
	{ .cmd_name = "labels",
	  .fn = subcommand_labels,
	  .docstring = "Manage issue and PR labels" },
	{ .cmd_name = "milestones",
	  .fn = subcommand_milestones,
	  .docstring = "Milestone handling" },
	{ .cmd_name = "pipelines",
	  .fn = subcommand_pipelines,
	  .docstring = "Gitlab CI management" },
	{ .cmd_name = "pulls",
	  .fn = subcommand_pulls,
	  .docstring = "Create, view and manage PRs" },
	{ .cmd_name = "releases",
	  .fn = subcommand_releases,
	  .docstring = "Manage releases of repositories" },
	{ .cmd_name = "repos",
	  .fn = subcommand_repos,
	  .docstring = "Remote Repository management" },
	{ .cmd_name = "snippets",
	  .fn = subcommand_snippets,
	  .docstring = "Fetch and list Gitlab snippets" },
	{ .cmd_name = "status",
	  .fn = subcommand_status,
	  .docstring = "General user status and notifications" },
	{ .cmd_name = "api",
	  .fn = subcommand_api,
	  .docstring = "Fetch plain JSON info from an API (for debugging purposes)" },
	{ .cmd_name = "version",
	  .fn = subcommand_version,
	  .docstring = "Print version" },
};

static void
usage(void)
{
	fprintf(stderr, "usage: gcli [options] subcommand\n\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "  -a account     Use the configured account instead of inferring it\n");
	fprintf(stderr, "  -r remote      Infer account from the given git remote\n");
	fprintf(stderr, "  -t type        Force the account type:\n");
	fprintf(stderr, "                    - github (default: github.com)\n");
	fprintf(stderr, "                    - gitlab (default: gitlab.com)\n");
	fprintf(stderr, "                    - gitea (default: codeberg.org)\n");
	fprintf(stderr, "  -c             Force colour and text formatting.\n");
	fprintf(stderr, "  -q             Be quiet. (Not implemented yet)\n\n");
	fprintf(stderr, "  -v             Be verbose.\n\n");
	fprintf(stderr, "SUBCOMMANDS:\n");
	for (size_t i = 0; i < ARRAY_SIZE(subcommands); ++i) {
		fprintf(stderr,
		        "  %-13.13s  %s\n",
		        subcommands[i].cmd_name,
		        subcommands[i].docstring);
	}
	fprintf(stderr, "\n");
	version();
	copyright();
}

/** The CMD global gcli context */
gcli_ctx *g_clictx = NULL;

int
main(int argc, char *argv[])
{
	char const *errmsg;

	errmsg = gcli_init(&g_clictx);
	if (errmsg)
		errx(1, "error: %s", errmsg);

	/* Parse first arguments */
	if (gcli_config_parse_args(g_clictx, &argc, &argv)) {
		usage();
		return EXIT_FAILURE;
	}

	/* Make sure we have a subcommand */
	if (argc == 0) {
		fprintf(stderr, "error: missing subcommand\n");
		usage();
		return EXIT_FAILURE;
	}

	/* Find and invoke the subcommand handler */
	for (size_t i = 0; i < ARRAY_SIZE(subcommands); ++i) {
		if (strcmp(subcommands[i].cmd_name, argv[0]) == 0)
			return subcommands[i].fn(argc, argv);
	}

	/* No subcommand matched */
	fprintf(stderr, "error: unknown subcommand %s\n", argv[0]);
	usage();

	return EXIT_FAILURE;
}
