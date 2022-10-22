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
subcommand_repos_create(int argc, char *argv[])
{
    int                       ch;
    gcli_repo_create_options  create_options = {0};
    gcli_repo                *created_repo   = NULL;

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
        }
    }

    argc -= optind;
    argv += optind;

    if (sn_sv_null(create_options.name))
        errx(1,
             "name cannot be empty. please set a repository "
             "name with -r/--name");

    created_repo = gcli_repo_create(&create_options);

    gcli_print_repos_table(OUTPUT_ORDER_UNSORTED, created_repo, 1);
    gcli_repos_free(created_repo, 1);

    return EXIT_SUCCESS;
}

static int
subcommand_repos(int argc, char *argv[])
{
    int                     ch, repos_size, n = 30;
    const char             *owner             = NULL;
    const char             *repo              = NULL;
    gcli_repo              *repos             = NULL;
    bool                    always_yes        = false;
    enum gcli_output_order  order             = OUTPUT_ORDER_UNSORTED;

    /* detect whether we wanna create a repo */
    if (argc > 1 && (strcmp(argv[1], "create") == 0)) {
        shift(&argc, &argv);
        return subcommand_repos_create(argc, argv);
    }

    const struct option options[] = {
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
            order = OUTPUT_ORDER_SORTED;
            break;
        case 'n': {
            char *endptr = NULL;
            n = strtol(optarg, &endptr, 10);
            if (endptr != (optarg + strlen(optarg)))
                err(1, "repos: cannot parse repo count");
        } break;
        case '?':
        default:
            usage();
        }
    }

    argc -= optind;
    argv += optind;

    /* List repos of the owner */
    if (argc == 0) {
        if (repo)
            errx(1, "repos: no actions specified");

        if (!owner)
            repos_size = gcli_get_own_repos(n, &repos);
        else
            repos_size = gcli_get_repos(owner, n, &repos);

        gcli_print_repos_table(order, repos, (size_t)repos_size);
        gcli_repos_free(repos, repos_size);
    } else {
        check_owner_and_repo(&owner, &repo);

        for (size_t i = 0; i < (size_t)argc; ++i) {
            const char *action = argv[i];

            if (strcmp(action, "delete") == 0) {
                delete_repo(always_yes, owner, repo);
            } else {
                errx(1, "repos: unknown action '%s'", action);
            }
        }
    }

    return EXIT_SUCCESS;
}

static int
subcommand_gist_get(int argc, char *argv[])
{
    shift(&argc, &argv); /* Discard the *get* */

    const char     *gist_id   = shift(&argc, &argv);
    const char     *file_name = shift(&argc, &argv);
    gcli_gist      *gist      = NULL;
    gcli_gist_file *file      = NULL;

    gist = gcli_get_gist(gist_id);

    for (size_t f = 0; f < gist->files_size; ++f) {
        if (sn_sv_eq_to(gist->files[f].filename, file_name)) {
            file = &gist->files[f];
            goto file_found;
        }
    }

    errx(1, "gists get: %s: no such file in gist with id %s",
         file_name, gist_id);

file_found:
    /* TODO: check if tty when file is large */
    /* TODO: HACK */

    if (isatty(STDOUT_FILENO) && (file->size >= 4 * 1024 * 1024))
        errx(1, "File is bigger than 4 MiB, refusing to print to stdout.");

    gcli_curl(stdout, file->url.data, file->type.data);
    return EXIT_SUCCESS;
}

static int
subcommand_gist_create(int argc, char *argv[])
{
    int            ch;
    gcli_new_gist  opts = {0};
    const char    *file = NULL;

    const struct option options[] = {
        { .name    = "file",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'r' },
        { .name    = "description",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'o' },
        {0},
    };

    while ((ch = getopt_long(argc, argv, "f:d:", options, NULL)) != -1) {
        switch (ch) {
        case 'f':
            file = optarg;
            break;
        case 'd':
            opts.gist_description = optarg;
            break;
        case '?':
        default:
            usage();
        }
    }

    argc -= optind;
    argv += optind;

    if (argc != 1)
        errx(1, "gists create: missing file name for gist");

    opts.file_name = shift(&argc, &argv);

    if (file) {
        if ((opts.file = fopen(file, "r")) == NULL)
            err(1, "gists create: cannot open file");
    } else {
        opts.file = stdin;
    }

    if (!opts.gist_description)
        opts.gist_description = "gcli paste";

    gcli_create_gist(opts);

    return EXIT_SUCCESS;
}

static int
subcommand_gist_delete(int argc, char *argv[])
{
    int         ch;
    bool        always_yes = false;
    const char *gist_id    = NULL;

    const struct option options[] = {
        { .name    = "yes",
          .has_arg = no_argument,
          .flag    = NULL,
          .val     = 'y' },
        {0},
    };

    while ((ch = getopt_long(argc, argv, "y", options, NULL)) != -1) {
        switch (ch) {
        case 'y':
            always_yes = true;
            break;
        case '?':
        default:
            usage();
        }
    }

    argc -= optind;
    argv += optind;

    gist_id = shift(&argc, &argv);
    gcli_delete_gist(gist_id, always_yes);

    return EXIT_SUCCESS;
}

static struct {
    const char *name;
    int (*fn)(int, char **);
} gist_subcommands[] = {
    { .name = "get",    .fn = subcommand_gist_get    },
    { .name = "create", .fn = subcommand_gist_create },
    { .name = "delete", .fn = subcommand_gist_delete },
};

static int
subcommand_gists(int argc, char *argv[])
{
    int                     ch;
    const char             *user       = NULL;
    gcli_gist              *gists      = NULL;
    int                     gists_size = 0;
    int                     count      = 30;
    enum gcli_output_order  order      = OUTPUT_ORDER_UNSORTED;

    for (size_t i = 0; i < ARRAY_SIZE(gist_subcommands); ++i) {
        if (argc > 1 && strcmp(argv[1], gist_subcommands[i].name) == 0) {
            argc -= 1;
            argv += 1;
            return gist_subcommands[i].fn(argc, argv);
        }
    }

    const struct option options[] = {
        { .name    = "user",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'u' },
        { .name    = "count",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'n' },
        { .name    = "sorted",
          .has_arg = no_argument,
          .flag    = NULL,
          .val     = 's' },
        {0},
    };

    while ((ch = getopt_long(argc, argv, "sn:u:", options, NULL)) != -1) {
        switch (ch) {
        case 'u':
            user = optarg;
            break;
        case 'n': {
            char *endptr = NULL;
            count        = strtol(optarg, &endptr, 10);
            if (endptr != (optarg + strlen(optarg)))
                err(1, "gists: cannot parse gists count");
        } break;
        case 's':
            order = OUTPUT_ORDER_SORTED;
            break;
        case '?':
        default:
            usage();
        }
    }

    argc -= optind;
    argv += optind;

    gists_size = gcli_get_gists(user, count, &gists);
    gcli_print_gists_table(order, gists, gists_size);
    return EXIT_SUCCESS;
}

static int
subcommand_snippet_get(int argc, char *argv[])
{
    argc -= 1;
    argv += 1;

    if (!argc)
        errx(1, "snippets get: expected ID of snippet to fetch");

    char *snippet_id = shift(&argc, &argv);

    if (argc)
        errx(1, "snippet get: trailing options");

    gcli_snippet_get(snippet_id);

    return EXIT_SUCCESS;
}

static int
subcommand_snippet_delete(int argc, char *argv[])
{
    argc -= 1;
    argv += 1;

    if (!argc)
        errx(1, "snippets delete: expected ID of snippet to delete");

    char *snippet_id = shift(&argc, &argv);

    if (argc)
        errx(1, "snippet delete: trailing options");

    gcli_snippet_delete(snippet_id);

    return EXIT_SUCCESS;
}

static struct snippet_subcommand {
    const char *name;
    int (*fn)(int argc, char *argv[]);
} snippet_subcommands[] = {
    { .name = "get",    .fn = subcommand_snippet_get    },
    { .name = "delete", .fn = subcommand_snippet_delete },
};

static int
subcommand_snippets(int argc, char *argv[])
{
    int                     ch;
    gcli_snippet           *snippets      = NULL;
    int                     snippets_size = 0;
    int                     count         = 30;
    enum gcli_output_order  order         = OUTPUT_ORDER_UNSORTED;

    for (size_t i = 0; i < ARRAY_SIZE(snippet_subcommands); ++i) {
        if (argc > 1 && strcmp(argv[1], snippet_subcommands[i].name) == 0) {
            argc -= 1;
            argv += 1;
            return snippet_subcommands[i].fn(argc, argv);
        }
    }

    const struct option options[] = {
        { .name    = "count",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'n' },
        { .name    = "sorted",
          .has_arg = no_argument,
          .flag    = NULL,
          .val     = 's' },
        {0},
    };

    while ((ch = getopt_long(argc, argv, "sn:", options, NULL)) != -1) {
        switch (ch) {
        case 'n': {
            char *endptr = NULL;
            count        = strtol(optarg, &endptr, 10);
            if (endptr != (optarg + strlen(optarg)))
                err(1, "snippets: cannot parse snippets count");
        } break;
        case 's':
            order = OUTPUT_ORDER_SORTED;
            break;
        case '?':
        default:
            usage();
        }
    }

    argc -= optind;
    argv += optind;

    snippets_size = gcli_snippets_get(count, &snippets);
    gcli_snippets_print(order, snippets, snippets_size);
    return EXIT_SUCCESS;
}

static int
subcommand_status(int argc, char *argv[])
{
    int   count  = 30;
    int   ch     = 0;
    char *endptr = NULL;
    int   mark   = 0;

    const struct option options[] = {
        { .name    = "count",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'n' },
        { .name    = "mark",
          .has_arg = no_argument,
          .flag    = &mark,
          .val     = 1 },
        {0}
    };

    while ((ch = getopt_long(argc, argv, "n:m", options, NULL)) != -1) {
        switch (ch) {
        case 'n': {
            count = strtol(optarg, &endptr, 10);
            if (endptr != optarg + strlen(optarg))
                err(1, "status: cannot parse parameter to -n");
        } break;
        case 'm': {
            mark = 1;
        } break;
        default:
            usage();
        }
    }

    argc -= optind;
    argv += optind;

    if (!mark) {
        gcli_status(count);
    } else {
        if (count != 30)
            warnx("ignoring -n/--count argument");

        if (argc > 1)
            errx(1, "error: too many arguments for marking notifications");

        if (argc < 1)
            errx(1, "error: missing notification id to mark as read");

        gcli_notification_mark_as_read(argv[0]);
    }

    return EXIT_SUCCESS;
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
