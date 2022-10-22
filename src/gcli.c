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

#include "config.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gcli/cmd.h>
#include <gcli/comments.h>
#include <gcli/config.h>
#include <gcli/curl.h>
#include <gcli/editor.h>
#include <gcli/forges.h>
#include <gcli/forks.h>
#include <gcli/gists.h>
#include <gcli/gitconfig.h>
#include <gcli/issues.h>
#include <gcli/labels.h>
#include <gcli/pulls.h>
#include <gcli/releases.h>
#include <gcli/repos.h>
#include <gcli/review.h>
#include <gcli/snippets.h>
#include <gcli/status.h>

#include <gcli/github/checks.h>
#include <gcli/gitlab/pipelines.h>

static void
usage(void)
{
    fprintf(stderr, "usage: gcli [options] [subcommand] [options] ...\n");
    version();
    exit(1);
}

static int
subcommand_ci(int argc, char *argv[])
{
    int         ch    = 0;
    const char *owner = NULL, *repo = NULL;
    const char *ref   = NULL;
    int         count = -1;     /* fetch all checks by default */

    /* Parse options */
    const struct option options[] = {
        {.name = "repo",  .has_arg = required_argument, .flag = NULL, .val = 'r'},
        {.name = "owner", .has_arg = required_argument, .flag = NULL, .val = 'o'},
        {.name = "count", .has_arg = required_argument, .flag = NULL, .val = 'c'},
        {0}
    };

    while ((ch = getopt_long(argc, argv, "n:o:r:", options, NULL)) != -1) {
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
        case '?':
        default:
            usage();
        }
    }

    argc -= optind;
    argv += optind;

    /* Check that we have exactly one left argument and print proper
     * error messages */
    if (argc < 1)
        errx(1, "error: missing ref");

    if (argc > 1)
        errx(1, "error: stray arguments");

    /* Save the ref */
    ref = argv[0];

    check_owner_and_repo(&owner, &repo);

    /* Make sure we are actually talking about a github remote because
     * we might be incorrectly inferring it */
    if (gcli_config_get_forge_type() != GCLI_FORGE_GITHUB)
        errx(1, "error: The ci subcommand only works for GitHub. "
             "Use gcli -t github ... to force a GitHub remote.");

    github_checks(owner, repo, ref, count);

    return EXIT_SUCCESS;
}

static int
subcommand_pipelines(int argc, char *argv[])
{
    int         ch    = 0;
    const char *owner = NULL, *repo = NULL;
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
        }
    }

    argc -= optind;
    argv += optind;

    if (pid > 0 && jid > 0)
        errx(1, "error: -p and -j are mutually exclusive");

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
        if (argc != 0)
            errx(1, "error: stray arguments");

        gitlab_pipeline_jobs(owner, repo, pid, count);
        return EXIT_SUCCESS;
    }

    /* if the user didn't specify the -j option to list jobs, list the
     * pipelines instead */
    if (jid < 0) {
        /* Make sure we are interpreting things correctly */
        if (argc != 0)
            errx(1, "error: stray arguments");

        gitlab_pipelines(owner, repo, count);
        return EXIT_SUCCESS;
    }

    /* At this point jid contains a (hopefully) valid job id */

    /* Definition of the action list */
    struct {
        const char *name;                               /* Name on the cli */
        void (*fn)(const char *, const char *, long);   /* Function to be invoked for this action */
    } job_actions[] = {
        { .name = "log",    .fn = gitlab_job_get_log },
        { .name = "status", .fn = gitlab_job_status  },
        { .name = "cancel", .fn = gitlab_job_cancel  },
        { .name = "retry",  .fn = gitlab_job_retry   },
    };

next_action:
    while (argc) {
        const char *action = shift(&argc, &argv);

        /* Find the action and invoke it */
        for (size_t i = 0; i < ARRAY_SIZE(job_actions); ++i) {
            if (strcmp(action, job_actions[i].name) == 0) {
                job_actions[i].fn(owner, repo, jid);
                goto next_action;
            }
        }

        errx(1, "error: unknown action '%s'", action);
    }

    return EXIT_SUCCESS;
}

static int
subcommand_version(int argc, char *argv[])
{
    (void) argc;
    (void) argv;
    version();
    return 0;
}

static struct subcommand {
    const char *cmd_name;
    int (*fn)(int, char **);
} subcommands[] = {
    { .cmd_name = "ci",        .fn = subcommand_ci        },
    { .cmd_name = "comment",   .fn = subcommand_comment   },
    { .cmd_name = "forks",     .fn = subcommand_forks     },
    { .cmd_name = "gists",     .fn = subcommand_gists     },
    { .cmd_name = "issues",    .fn = subcommand_issues    },
    { .cmd_name = "labels",    .fn = subcommand_labels    },
    { .cmd_name = "pipelines", .fn = subcommand_pipelines },
    { .cmd_name = "pulls",     .fn = subcommand_pulls     },
    { .cmd_name = "releases",  .fn = subcommand_releases  },
    { .cmd_name = "repos",     .fn = subcommand_repos     },
    { .cmd_name = "snippets",  .fn = subcommand_snippets  },
    { .cmd_name = "status",    .fn = subcommand_status    },
    { .cmd_name = "version",   .fn = subcommand_version   },
};

int
main(int argc, char *argv[])
{
    gcli_config_init(&argc, &argv);

    if (argc == 0)
        errx(1, "error: missing subcommand");

    for (size_t i = 0; i < ARRAY_SIZE(subcommands); ++i) {
        if (strcmp(subcommands[i].cmd_name, argv[0]) == 0)
            return subcommands[i].fn(argc, argv);
    }

    /* No subcommand matched */
    fprintf(stderr, "error: unknown subcommand %s\n", argv[0]);
    usage();

    return 42;
}
