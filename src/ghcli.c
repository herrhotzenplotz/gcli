/*
 * Copyright 2021 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ghcli/comments.h>
#include <ghcli/config.h>
#include <ghcli/curl.h>
#include <ghcli/editor.h>
#include <ghcli/forges.h>
#include <ghcli/forks.h>
#include <ghcli/gists.h>
#include <ghcli/gitconfig.h>
#include <ghcli/issues.h>
#include <ghcli/labels.h>
#include <ghcli/pulls.h>
#include <ghcli/releases.h>
#include <ghcli/repos.h>
#include <ghcli/review.h>
#include <ghcli/snippets.h>
#include <ghcli/status.h>

#include <sn/sn.h>

static char *
shift(int *argc, char ***argv)
{
    if (*argc == 0)
        errx(1, "error: Not enough arguments");

    (*argc)--;
    return *((*argv)++);
}

static void
version(void)
{
    fprintf(
        stderr,
        "ghcli version "GHCLI_VERSION_STRING
        " - a command line utility to interact with GitHub\n"
        "Copyright 2021 Nico Sonack <nsonack@herrhotzenplotz.de>\n"
        "This program is licensed under the BSD2CLAUSE license. You should\n"
        "received a copy of it with its distribution.\n");
}

static void
usage(void)
{
    fprintf(stderr, "usage: ghcli [subcommand] [options] ...\n");
    version();
    exit(1);
}

static void
check_owner_and_repo(const char **owner, const char **repo)
{
    /* If no remote was specified, try to autodetect */
    if ((*owner == NULL) != (*repo == NULL))
        errx(1, "error: missing either explicit owner or repo");

    if (*owner == NULL)
        ghcli_config_get_repo(owner, repo);
}

static sn_sv
pr_try_derive_head(void)
{
    sn_sv account = {0};
    sn_sv branch  = {0};

    if (!(account = ghcli_config_get_account()).length)
        errx(1,
             "error: Cannot derive PR head. Please specify --from or set the\n"
             "       account in the users ghcli config file.");

    if (!(branch = ghcli_gitconfig_get_current_branch()).length)
        errx(1,
             "error: Cannot derive PR head. Please specify --from or, if you\n"
             "       are in »detached HEAD« state, checkout the branch you \n"
             "       want to pull request.");

    return sn_sv_fmt(SV_FMT":"SV_FMT, SV_ARGS(account), SV_ARGS(branch));
}

/**
 * Create a pull request
 */
static int
subcommand_issue_create(int argc, char *argv[])
{
    /* we'll use getopt_long here to parse the arguments */
    int                       ch;
    ghcli_submit_issue_options opts   = {0};

    const struct option options[] = {
        { .name    = "owner",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'o' },
        { .name    = "repo",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'r' },
        { .name    = "yes",
          .has_arg = no_argument,
          .flag    = NULL,
          .val     = 'y' },
        {0},
    };

    while ((ch = getopt_long(argc, argv, "o:r:", options, NULL)) != -1) {
        switch (ch) {
        case 'o':
            opts.owner = SV(optarg);
            break;
        case 'r':
            opts.repo = SV(optarg);
            break;
        case 'y':
            opts.always_yes = true;
        default:
            usage();
        }
    }

    argc -= optind;
    argv += optind;

    if (!opts.owner.length || !opts.repo.length) {
        ghcli_config_get_upstream_parts(&opts.owner, &opts.repo);
        if (!opts.owner.length || !opts.repo.length)
            errx(1,
                 "error: Target repo for the issue to be created "
                 "in is missing. Please either specify '-o owner' "
                 "and '-r repo' or set pr.upstream in .ghcli.");
    }

    if (argc != 1)
        errx(1, "error: Expected one argument for issue title");

    opts.title = SV(argv[0]);

    ghcli_issue_submit(opts);

    return EXIT_SUCCESS;
}

/**
 * Create a pull request
 */
static int
subcommand_pull_create(int argc, char *argv[])
{
    /* we'll use getopt_long here to parse the arguments */
    int                       ch;
    ghcli_submit_pull_options opts   = {0};

    const struct option options[] = {
        { .name = "from",
          .has_arg = required_argument,
          .flag = NULL,
          .val = 'f' },
        { .name = "to",
          .has_arg = required_argument,
          .flag = NULL,
          .val = 't' },
        { .name = "in",
          .has_arg = required_argument,
          .flag = NULL,
          .val = 'i' },
        { .name = "draft",
          .has_arg = no_argument,
          .flag = &opts.draft,
          .val = 1   },
        {0},
    };

    while ((ch = getopt_long(argc, argv, "yf:t:di:", options, NULL)) != -1) {
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
        case 'i':
            opts.in    = SV(optarg);
            break;
        case 'y':
            opts.always_yes = true;
            break;
        default:
            usage();
        }
    }

    argc -= optind;
    argv += optind;

    if (!opts.from.length)
        opts.from = pr_try_derive_head();

    if (!opts.to.length) {
        if (!(opts.to = ghcli_config_get_base()).length)
            errx(1,
                 "error: PR base is missing. Please either specify "
                 "--to branch-name or set pr.base in .ghcli.");
    }

    if (!opts.in.length) {
        if (!(opts.in = ghcli_config_get_upstream()).length)
            errx(1, "error: PR target repo is missing. Please either "
                 "specify --in owner/repo or set pr.upstream in .ghcli.");
    }

    if (argc != 1)
        errx(1, "error: Missing title to PR");

    opts.title = SV(argv[0]);

    ghcli_pr_submit(opts);

    return EXIT_SUCCESS;
}

/**
 * Create a comment
 */
static int
subcommand_comment(int argc, char *argv[])
{
    int                       ch, target_id  = -1;
    const char               *repo       = NULL, *owner = NULL;
    bool                      always_yes = false;
    enum comment_target_type  target_type;

    const struct option options[] = {
        { .name    = "yes",
          .has_arg = no_argument,
          .flag    = NULL,
          .val     = 'y' },
        { .name    = "repo",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'r' },
        { .name    = "owner",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'o' },
        { .name    = "issue",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'i' },
        { .name    = "pull",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'p' },
        {0},
    };

    while ((ch = getopt_long(argc, argv, "yr:o:i:p:", options, NULL)) != -1) {
        switch (ch) {
        case 'r':
            repo = optarg;
            break;
        case 'o':
            owner = optarg;
            break;
        case 'p':
            target_type = PR_COMMENT;
            goto parse_target_id;
        case 'i':
            target_type = ISSUE_COMMENT;
        parse_target_id: {
            char *endptr;
            target_id = strtoul(optarg, &endptr, 10);
            if (endptr != optarg + strlen(optarg))
                err(1, "error: Cannot parse issue/PR number");
        } break;
        case 'y':
            always_yes = true;
            break;
        default:
            usage();
        }
    }

    argc -= optind;
    argv += optind;

    check_owner_and_repo(&owner, &repo);

    if (target_id < 0)
        errx(1, "error: missing issue/PR number (use -i/-p)");

    ghcli_comment_submit((ghcli_submit_comment_opts) {
            .owner       = owner,
            .repo        = repo,
            .target_type = target_type,
            .target_id   = target_id,
            .always_yes  = always_yes,
    });

    return EXIT_SUCCESS;
}

static int
subcommand_pulls(int argc, char *argv[])
{
    char                    *endptr     = NULL;
    const char              *owner      = NULL;
    const char              *repo       = NULL;
    ghcli_pull              *pulls      = NULL;
    int                      ch         = 0;
    int                      pr         = -1;
    int                      pulls_size = 0;
    int                      n          = 30; /* how many prs to fetch at least */
    bool                     all        = false;
    enum ghcli_output_order  order      = OUTPUT_ORDER_UNSORTED;

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
    while ((ch = getopt_long(argc, argv, "n:o:r:p:as", options, NULL)) != -1) {
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
            order = OUTPUT_ORDER_SORTED;
        } break;
        case '?':
        default:
            usage();
        }
    }

    argc -= optind;
    argv += optind;

    check_owner_and_repo(&owner, &repo);

    /* In case no explicit PR number was specified, list all
     * open PRs and exit */
    if (pr < 0) {
        pulls_size = ghcli_get_prs(owner, repo, all, n, &pulls);
        ghcli_print_pr_table(stdout, order, pulls, pulls_size);

        ghcli_pulls_free(pulls, pulls_size);
        free(pulls);

        return EXIT_SUCCESS;
    }

    /* If a PR number was given, require -a to be unset */
    if (all)
        errx(1, "error: -a cannot be combined with operations on a PR");

    /* we have an explicit PR number, so execute all operations the
     * user has given */
    while (argc > 0) {
        const char *operation = shift(&argc, &argv);

        if (strcmp(operation, "diff") == 0) {
            ghcli_print_pr_diff(stdout, owner, repo, pr);
        } else if (strcmp(operation, "summary") == 0
                   || strcmp(operation, "status") == 0) {
            ghcli_pr_summary(stdout, owner, repo, pr);
        } else if (strcmp(operation, "comments") == 0) {
            ghcli_pull_comments(stdout, owner, repo, pr);
        } else if (strcmp(operation, "merge") == 0) {
            ghcli_pr_merge(stdout, owner, repo, pr);
        } else if (strcmp(operation, "close") == 0) {
            ghcli_pr_close(owner, repo, pr);
        } else if (strcmp(operation, "reopen") == 0) {
            ghcli_pr_reopen(owner, repo, pr);
        } else if (strcmp(operation, "reviews") == 0) {
            /* list reviews */
            ghcli_pr_review *reviews      = NULL;
            size_t           reviews_size = ghcli_review_get_reviews(
                owner, repo, pr, &reviews);
            ghcli_review_print_review_table(stdout, reviews, reviews_size);
            ghcli_review_reviews_free(reviews, reviews_size);
        } else {
            errx(1, "error: unknown operation %s", operation);
        }
    }

    return EXIT_SUCCESS;
}

static int
subcommand_issues(int argc, char *argv[])
{
    ghcli_issue             *issues      = NULL;
    int                      issues_size = 0;
    const char              *owner       = NULL;
    const char              *repo        = NULL;
    char                    *endptr      = NULL;
    int                      ch          = 0;
    int                      issue       = -1;
    int                      n           = 30;
    bool                     all         = false;
    enum ghcli_output_order  order       = OUTPUT_ORDER_UNSORTED;

    /* detect whether we wanna create an issue */
    if (argc > 1 && (strcmp(argv[1], "create") == 0)) {
        shift(&argc, &argv);
        return subcommand_issue_create(argc, argv);
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
        { .name    = "repo",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'r' },
        { .name    = "owner",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'o' },
        { .name    = "issue",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'i' },
        { .name    = "count",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'n'
        },
        {0},
    };

    /* parse options */
    while ((ch = getopt_long(argc, argv, "+sn:o:r:i:a", options, NULL)) != -1) {
        switch (ch) {
        case 'o':
            owner = optarg;
            break;
        case 'r':
            repo = optarg;
            break;
        case 'i': {
            issue = strtol(optarg, &endptr, 10);
            if (endptr != (optarg + strlen(optarg)))
                err(1, "error: cannot parse issue number");

            if (issue < 0)
                errx(1, "error: issue number is out of range");
        } break;
        case 'n': {
            n = strtol(optarg, &endptr, 10);
            if (endptr != (optarg + strlen(optarg)))
                err(1, "error: cannot parse issue count");

            if (n < -1)
                errx(1, "error: issue count is out of range");
        } break;
        case 'a':
            all = true;
            break;
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


    check_owner_and_repo(&owner, &repo);

    /* No issue number was given, so list all open issues */
    if (issue < 0) {
        issues_size = ghcli_get_issues(owner, repo, all, n, &issues);
        ghcli_print_issues_table(stdout, order, issues, issues_size);

        ghcli_issues_free(issues, issues_size);
        return EXIT_SUCCESS;
    }

    /* require -a to not be set */
    if (all)
        errx(1, "-a cannot be combined with operations on an issue");

    /* execute all operations on the given issue */
    while (argc > 0) {
        const char *operation = shift(&argc, &argv);

        if (strcmp("comments", operation) == 0) {
            ghcli_issue_comments(stdout, owner, repo, issue);
        } else if (strcmp("summary", operation) == 0
                   || strcmp("status", operation) == 0) {
            ghcli_issue_summary(stdout, owner, repo, issue);
        } else if (strcmp("close", operation) == 0) {
            ghcli_issue_close(owner, repo, issue);
        } else if (strcmp("reopen", operation) == 0) {
            ghcli_issue_reopen(owner, repo, issue);
        } else if (strcmp("assign", operation) == 0) {
            const char *assignee = shift(&argc, &argv);
            ghcli_issue_assign(owner, repo, issue, assignee);
        } else if (strcmp("labels", operation) == 0) {
            const char **add_labels         = NULL;
            size_t       add_labels_size    = 0;
            const char **remove_labels      = NULL;
            size_t       remove_labels_size = 0;

            if (argc == 0)
                errx(1, "error: expected label operations");

            /* Collect add/delete labels */
            while (argc > 0) {
                if (strcmp(*argv, "--add") == 0) {
                    shift(&argc, &argv);

                    add_labels = realloc(
                        add_labels,
                        (add_labels_size + 1) * sizeof(*add_labels));
                    add_labels[add_labels_size++] = shift(&argc, &argv);
                } else if (strcmp(*argv, "--remove") == 0) {
                    shift(&argc, &argv);

                    remove_labels = realloc(
                        remove_labels,
                        (remove_labels_size + 1) * sizeof(*remove_labels));
                    remove_labels[remove_labels_size++] = shift(&argc, &argv);
                } else {
                    break;
                }
            }

            /* actually go about deleting and adding the labels */
            if (add_labels_size)
                ghcli_issue_add_labels(owner, repo, issue,
                                       add_labels, add_labels_size);
            if (remove_labels_size)
                ghcli_issue_remove_labels(owner, repo, issue,
                                          remove_labels, remove_labels_size);

            free(add_labels);
            free(remove_labels);
        } else {
            errx(1, "error: unknown operation %s", operation);
        }
    }

    return EXIT_SUCCESS;
}

static int
subcommand_forks_create(int argc, char *argv[])
{
    int         ch;
    const char *owner      = NULL, *repo = NULL, *in = NULL;
    bool        always_yes = false;

    const struct option options[] = {
        { .name    = "repo",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'r' },
        { .name    = "owner",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'o' },
        { .name    = "into",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'i' },
        { .name    = "yes",
          .has_arg = no_argument,
          .flag    = NULL,
          .val     = 'y' },
        {0},
    };

    while ((ch = getopt_long(argc, argv, "yo:r:i:", options, NULL)) != -1) {
        switch (ch) {
        case 'o':
            owner = optarg;
            break;
        case 'r':
            repo = optarg;
            break;
        case 'i':
            in = optarg;
            break;
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

    check_owner_and_repo(&owner, &repo);

    ghcli_fork_create(owner, repo, in);

    if (!always_yes) {
        if (!sn_yesno("Do you want to add a remote for the fork?"))
            return EXIT_SUCCESS;
    }

    if (!in)
        in = sn_sv_to_cstr(ghcli_config_get_account());

    ghcli_gitconfig_add_fork_remote(in, repo);

    return EXIT_SUCCESS;
}

static void
delete_repo(bool always_yes, const char *owner, const char *repo)
{
    bool delete = false;

    if (!always_yes) {
        delete = sn_yesno(
            "Are you sure you want to delete %s/%s?",
            owner, repo);
    } else {
        delete = true;
    }

    if (delete)
        ghcli_repo_delete(owner, repo);
    else
        errx(1, "Operation aborted");
}

static int
subcommand_repos(int argc, char *argv[])
{
    int                      ch, repos_size, n = 30;
    const char              *owner             = NULL;
    const char              *repo              = NULL;
    ghcli_repo              *repos             = NULL;
    bool                     always_yes        = false;
    enum ghcli_output_order  order             = OUTPUT_ORDER_UNSORTED;

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
            repos_size = ghcli_get_own_repos(n, &repos);
        else
            repos_size = ghcli_get_repos(owner, n, &repos);

        ghcli_print_repos_table(stdout, order, repos, (size_t)repos_size);
        ghcli_repos_free(repos, repos_size);
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

    const char      *gist_id   = shift(&argc, &argv);
    const char      *file_name = shift(&argc, &argv);
    ghcli_gist      *gist      = NULL;
    ghcli_gist_file *file      = NULL;

    gist = ghcli_get_gist(gist_id);

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

    ghcli_curl(stdout, file->url.data, file->type.data);
    return EXIT_SUCCESS;
}

static int
subcommand_gist_create(int argc, char *argv[])
{
    int             ch;
    ghcli_new_gist  opts = {0};
    const char     *file = NULL;

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
        opts.gist_description = "ghcli paste";

    ghcli_create_gist(opts);

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
    ghcli_delete_gist(gist_id, always_yes);

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
    int                      ch;
    const char              *user       = NULL;
    ghcli_gist              *gists      = NULL;
    int                      gists_size = 0;
    int                      count      = 30;
    enum ghcli_output_order  order      = OUTPUT_ORDER_UNSORTED;

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

    gists_size = ghcli_get_gists(user, count, &gists);
    ghcli_print_gists_table(stdout, order, gists, gists_size);
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

    ghcli_snippet_get(snippet_id);

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

    ghcli_snippet_delete(snippet_id);

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
    int                      ch;
    ghcli_snippet           *snippets      = NULL;
    int                      snippets_size = 0;
    int                      count         = 30;
    enum ghcli_output_order  order         = OUTPUT_ORDER_UNSORTED;

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

    snippets_size = ghcli_snippets_get(count, &snippets);
    ghcli_snippets_print(stdout, order, snippets, snippets_size);
    return EXIT_SUCCESS;
}

static int
subcommand_forks(int argc, char *argv[])
{
    ghcli_fork              *forks      = NULL;
    const char              *owner      = NULL, *repo = NULL;
    int                      forks_size = 0;
    int                      ch         = 0;
    int                      count      = 30;
    bool                     always_yes = false;
    enum ghcli_output_order  order      = OUTPUT_ORDER_UNSORTED;

    /* detect whether we wanna create a fork */
    if (argc > 1 && (strcmp(argv[1], "create") == 0)) {
        shift(&argc, &argv);
        return subcommand_forks_create(argc, argv);
    }

    const struct option options[] = {
        { .name    = "repo",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'r' },
        { .name    = "owner",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'o' },
        { .name    = "count",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'n' },
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
        case 'n': {
            char *endptr = NULL;
            count        = strtol(optarg, &endptr, 10);
            if (endptr != (optarg + strlen(optarg)))
                err(1, "forks: unable to parse forks count argument");
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

    check_owner_and_repo(&owner, &repo);

    if (argc == 0) {
        forks_size = ghcli_get_forks(owner, repo, count, &forks);
        ghcli_print_forks(stdout, order, forks, forks_size);
        return EXIT_SUCCESS;
    }

    for (size_t i = 0; i < (size_t)argc; ++i) {
        const char *action = argv[i];

        if (strcmp(action, "delete") == 0) {
            delete_repo(always_yes, owner, repo);
        } else {
            errx(1, "forks: unknown action '%s'", action);
        }
    }

    return EXIT_SUCCESS;
}

static void
releasemsg_init(FILE *f, void *_data)
{
    const ghcli_new_release *info = _data;

    fprintf(
        f,
        "# Enter your release notes below, save and exit.\n"
        "# All lines with a leading '#' are discarded and will not\n"
        "# appear in the final release note.\n"
        "#       IN : %s/%s\n"
        "# TAG NAME : %s\n"
        "#     NAME : %s\n",
        info->owner, info->repo, info->tag, info->name);
}

static sn_sv
get_release_message(const ghcli_new_release *info)
{
    return ghcli_editor_get_user_message(releasemsg_init, (void *)info);
}

static int
subcommand_releases_create(int argc, char *argv[])
{
    ghcli_new_release release     = {0};
    int               ch;
    bool              always_yes  = false;

    const struct option options[] = {
        { .name    = "yes",
          .has_arg = no_argument,
          .flag    = NULL,
          .val     = 'y' },
        { .name    = "draft",
          .has_arg = no_argument,
          .flag    = NULL,
          .val     = 'd' },
        { .name    = "prerelease",
          .has_arg = no_argument,
          .flag    = NULL,
          .val     = 'p' },
        { .name    = "name",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'n' },
        { .name    = "tag",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 't' },
        { .name    = "commitish",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'c' },
        { .name    = "repo",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'r' },
        { .name    = "owner",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'o' },
        { .name    = "asset",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'a' },
        {0},
    };

    while ((ch = getopt_long(argc, argv, "ydpn:t:c:r:o:a:",
                             options, NULL)) != -1) {
        switch (ch) {
        case 'd':
            release.draft = true;
            break;
        case 'p':
            release.prerelease = true;
            break;
        case 'n':
            release.name = optarg;
            break;
        case 't':
            release.tag = optarg;
            break;
        case 'c':
            release.commitish = optarg;
            break;
        case 'r':
            release.repo = optarg;
            break;
        case 'o':
            release.owner = optarg;
            break;
        case 'a': {
            ghcli_release_asset asset = {
                .path  = optarg,
                .name  = optarg,
                .label = "unused",
            };
            ghcli_release_push_asset(&release, asset);
        } break;
        case 'y': {
            always_yes = true;
        } break;
        default:
            usage();
        }
    }

    argc -= optind;
    argv += optind;

    check_owner_and_repo(&release.owner, &release.repo);

    if (!release.tag)
        errx(1, "releases create: missing tag name");

    release.body = get_release_message(&release);

    if (!always_yes)
        if (!sn_yesno("Do you want to create this release?"))
            errx(1, "Aborted by user");

    ghcli_create_release(&release);

    return EXIT_SUCCESS;
}

static int
subcommand_releases_delete(int argc, char *argv[])
{
    int         ch;
    const char *owner      = NULL, *repo = NULL;
    bool        always_yes = false;

    const struct option options[] = {
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
        {0}
    };

    while ((ch = getopt_long(argc, argv, "yo:r:", options, NULL)) != -1) {
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
        case '?':
        default:
            usage();
        }
    }

    argc -= optind;
    argv += optind;

    check_owner_and_repo(&owner, &repo);

    if (argc != 1)
        errx(1, "releases delete: missing release id");

    if (!always_yes)
        if (!sn_yesno("Are you sure you want to delete this release?"))
            errx(1, "Aborted by user");

    ghcli_delete_release(owner, repo, argv[0]);

    return EXIT_SUCCESS;
}

static struct {
    const char *name;
    int (*fn)(int, char **);
} releases_subcommands[] = {
    { .name = "delete", .fn = subcommand_releases_delete },
    { .name = "create", .fn = subcommand_releases_create },
};

static int
subcommand_releases(int argc, char *argv[])
{
    int                      ch;
    int                      releases_size;
    int                      count    = 30;
    const char              *owner    = NULL;
    const char              *repo     = NULL;
    ghcli_release           *releases = NULL;
    enum ghcli_output_order  order    = OUTPUT_ORDER_UNSORTED;

    if (argc > 1) {
        for (size_t i = 0; i < ARRAY_SIZE(releases_subcommands); ++i) {
            if (strcmp(releases_subcommands[i].name, argv[1]) == 0)
                return releases_subcommands[i].fn(argc - 1, argv + 1);
        }
    }

    /* List releases if none of the subcommands matched */

    const struct option options[] = {
        { .name    = "repo",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'r' },
        { .name    = "owner",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'o' },
        { .name    = "count",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'n' },
        { .name    = "sorted",
          .has_arg = no_argument,
          .flag    = NULL,
          .val     = 's' },
        {0}
    };

    while ((ch = getopt_long(argc, argv, "sn:o:r:", options, NULL)) != -1) {
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
                err(1, "releases: cannot parse release count");
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

    check_owner_and_repo(&owner, &repo);

    releases_size = ghcli_get_releases(owner, repo, count, &releases);
    ghcli_print_releases(stdout, order, releases, releases_size);
    ghcli_free_releases(releases, releases_size);

    return EXIT_SUCCESS;
}

static int
subcommand_labels_delete(int argc, char *argv[])
{
    int         ch;
    const char *owner             = NULL, *repo = NULL;
    const struct option options[] = {
        {.name = "repo",  .has_arg = required_argument, .val = 'r'},
        {.name = "owner", .has_arg = required_argument, .val = 'o'},
        {0},
    };

    while ((ch = getopt_long(argc, argv, "n:o:r:", options, NULL)) != -1) {
        switch (ch) {
        case 'o':
            owner = optarg;
            break;
        case 'r':
            repo = optarg;
            break;
        case '?':
        default:
            usage();
        }
    }

    argc -= optind;
    argv += optind;

    check_owner_and_repo(&owner, &repo);

    if (argc != 1)
        errx(1, "labels: missing label to delete");

    ghcli_delete_label(owner, repo, argv[0]);

    return EXIT_SUCCESS;
}

static int
subcommand_labels_create(int argc, char *argv[])
{
    ghcli_label  label = {0};
    const char  *owner = NULL, *repo = NULL;
    int          ch;

    const struct option options[] = {
        {.name = "repo",        .has_arg = required_argument, .val = 'r'},
        {.name = "owner",       .has_arg = required_argument, .val = 'o'},
        {.name = "name",        .has_arg = required_argument, .val = 'n'},
        {.name = "color",       .has_arg = required_argument, .val = 'c'},
        {.name = "description", .has_arg = required_argument, .val = 'd'},
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
        case 'c': {
            char *endptr = NULL;
            label.color = strtol(optarg, &endptr, 16);
            if (endptr != (optarg + strlen(optarg)))
                err(1, "labels: cannot parse color");
        } break;
        case 'd': {
            label.description = optarg;
        } break;
        case 'n': {
            label.name = optarg;
        } break;
        case '?':
        default:
            usage();
        }
    }

    argc -= optind;
    argv += optind;

    check_owner_and_repo(&owner, &repo);

    if (!label.name)
        errx(1, "error: missing name for label");

    if (!label.description)
        errx(1, "error: missing description for label");

    ghcli_create_label(owner, repo, &label);
    ghcli_print_labels(&label, 1);

    return EXIT_SUCCESS;
}

static struct {
    const char *name;
    int (*fn)(int, char **);
} labels_subcommands[] = {
    { .name = "delete", .fn = subcommand_labels_delete },
    { .name = "create", .fn = subcommand_labels_create },
};

static int
subcommand_labels(int argc, char *argv[])
{
    int          count = 30;
    int          ch;
    const char  *owner = NULL, *repo = NULL;
    size_t       labels_count;
    ghcli_label *labels;

    const struct option options[] = {
        {.name = "repo",  .has_arg = required_argument, .flag = NULL, .val = 'r'},
        {.name = "owner", .has_arg = required_argument, .flag = NULL, .val = 'o'},
        {.name = "count", .has_arg = required_argument, .flag = NULL, .val = 'c'},
        {0}
    };

    if (argc > 1) {
        for (size_t i = 0; i < ARRAY_SIZE(releases_subcommands); ++i) {
            if (strcmp(labels_subcommands[i].name, argv[1]) == 0)
                return labels_subcommands[i].fn(argc - 1, argv + 1);
        }
    }

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
                err(1, "labels: cannot parse label count");
        } break;
        case '?':
        default:
            usage();
        }
    }

    argc -= optind;
    argv += optind;

    check_owner_and_repo(&owner, &repo);

    labels_count = ghcli_get_labels(owner, repo, count, &labels);

    ghcli_print_labels(labels, labels_count);
    ghcli_free_labels(labels, labels_count);

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
        ghcli_status(count);
    } else {
        if (count != 30)
            warnx("ignoring -n/--count argument");

        if (argc > 1)
            errx(1, "error: too many arguments for marking notifications");

        if (argc < 1)
            errx(1, "error: missing notification id to mark as read");

        ghcli_notification_mark_as_read(argv[0]);
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
    { .cmd_name = "comment",  .fn = subcommand_comment  },
    { .cmd_name = "forks",    .fn = subcommand_forks    },
    { .cmd_name = "gists",    .fn = subcommand_gists    },
    { .cmd_name = "issues",   .fn = subcommand_issues   },
    { .cmd_name = "labels",   .fn = subcommand_labels   },
    { .cmd_name = "pulls",    .fn = subcommand_pulls    },
    { .cmd_name = "releases", .fn = subcommand_releases },
    { .cmd_name = "repos",    .fn = subcommand_repos    },
    { .cmd_name = "snippets", .fn = subcommand_snippets },
    { .cmd_name = "status",   .fn = subcommand_status   },
    { .cmd_name = "version",  .fn = subcommand_version  },
};

int
main(int argc, char *argv[])
{
    ghcli_config_init(&argc, &argv);

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
