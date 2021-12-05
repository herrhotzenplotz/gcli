/*
 * Copyright 2021 Nico Sonack <nsonack@outlook.com>
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
#include <ghcli/forks.h>
#include <ghcli/gists.h>
#include <ghcli/gitconfig.h>
#include <ghcli/issues.h>
#include <ghcli/pulls.h>
#include <ghcli/repos.h>
#include <ghcli/review.h>
#include <ghcli/releases.h>

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
        "Copyright 2021 Nico Sonack <nsonack@outlook.com>\n"
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
check_org_and_repo(const char **org, const char **repo)
{
    /* If no remote was specified, try to autodetect */
    if ((*org == NULL) != (*repo == NULL))
        errx(1, "error: missing either explicit org or repo");

    if (*org == NULL)
        ghcli_gitconfig_get_repo(org, repo);
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
        { .name    = "in",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'i' },
        { .name    = "yes",
          .has_arg = no_argument,
          .flag    = NULL,
          .val     = 'y' },
        {0},
    };

    while ((ch = getopt_long(argc, argv, "i:", options, NULL)) != -1) {
        switch (ch) {
        case 'i':
            opts.in    = SV(optarg);
            break;
        case 'y':
            opts.always_yes = true;
        default:
            usage();
        }
    }

    argc -= optind;
    argv += optind;

    if (!opts.in.length) {
        if (!(opts.in = ghcli_config_get_upstream()).length)
            errx(1,
                 "error: Target repo for the issue to be created "
                 "in is missing. Please either specify '--in org/repo'"
                 " or set pr.upstream in .ghcli.");
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
                 "specify --in org/repo or set pr.upstream in .ghcli.");
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
    int   ch, issue = -1;
    const char *repo  = NULL, *org = NULL;
    bool  always_yes = false;

    const struct option options[] = {
        { .name    = "yes",
          .has_arg = no_argument,
          .flag    = NULL,
          .val     = 'y' },
        { .name    = "repo",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'r' },
        { .name    = "org",
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
            org = optarg;
            break;
        case 'p':
        case 'i': {
            char *endptr;
            issue = strtoul(optarg, &endptr, 10);
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

    check_org_and_repo(&org, &repo);

    if (issue < 0)
        errx(1, "error: missing issue/PR number (use -i)");

    ghcli_comment_submit((ghcli_submit_comment_opts) {
            .org        = org,
            .repo       = repo,
            .issue      = issue,
            .always_yes = always_yes,
    });

    return EXIT_SUCCESS;
}

static int
subcommand_pulls(int argc, char *argv[])
{
    char       *endptr     = NULL;
    const char *org        = NULL;
    const char *repo       = NULL;
    ghcli_pull *pulls      = NULL;
    int         ch         = 0;
    int         pr         = -1;
    int         pulls_size = 0;
    bool        all        = false;

    /* detect whether we wanna create a PR */
    if (argc > 1 && (strcmp(argv[1], "create") == 0)) {
        shift(&argc, &argv);
        return subcommand_pull_create(argc, argv);
    }

    /* Parse commandline options */
    while ((ch = getopt(argc, argv, "o:r:p:a")) != -1) {
        switch (ch) {
        case 'o':
            org = optarg;
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
        case 'a': {
            all = true;
        } break;
        case '?':
        default:
            usage();
        }
    }

    argc -= optind;
    argv += optind;

    check_org_and_repo(&org, &repo);

    /* In case no explicit PR number was specified, list all
     * open PRs and exit */
    if (pr < 0) {
        pulls_size = ghcli_get_prs(org, repo, all, &pulls);
        ghcli_print_pr_table(stdout, pulls, pulls_size);

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

        if (strcmp(operation, "diff") == 0)
            ghcli_print_pr_diff(stdout, org, repo, pr);
        else if (strcmp(operation, "summary") == 0)
            ghcli_pr_summary(stdout, org, repo, pr);
        else if (strcmp(operation, "comments") == 0)
            ghcli_issue_comments(stdout, org, repo, pr);
        else if (strcmp(operation, "merge") == 0)
            ghcli_pr_merge(stdout, org, repo, pr);
        else if (strcmp(operation, "close") == 0)
            ghcli_pr_close(org, repo, pr);
        else if (strcmp(operation, "reopen") == 0)
            ghcli_pr_reopen(org, repo, pr);
        else
            errx(1, "error: unknown operation %s", operation);
    }

    return EXIT_SUCCESS;
}

static int
subcommand_issues(int argc, char *argv[])
{
    ghcli_issue *issues      = NULL;
    int          issues_size = 0;
    const char  *org         = NULL;
    const char  *repo        = NULL;
    char        *endptr      = NULL;
    int          ch          = 0;
    int          issue       = -1;
    bool         all         = false;

    /* detect whether we wanna create an issue */
    if (argc > 1 && (strcmp(argv[1], "create") == 0)) {
        shift(&argc, &argv);
        return subcommand_issue_create(argc, argv);
    }

    /* parse options */
    while ((ch = getopt(argc, argv, "o:r:i:a")) != -1) {
        switch (ch) {
        case 'o':
            org = optarg;
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
        case 'a':
            all = true;
            break;
        case '?':
        default:
            usage();
        }
    }

    argc -= optind;
    argv += optind;


    check_org_and_repo(&org, &repo);

    /* No issue number was given, so list all open issues */
    if (issue < 0) {
        issues_size = ghcli_get_issues(org, repo, all, &issues);
        ghcli_print_issues_table(stdout, issues, issues_size);

        ghcli_issues_free(issues, issues_size);
        return EXIT_SUCCESS;
    }

    /* require -a to not be set */
    if (all)
        errx(1, "-a cannot be combined with operations on an issue");

    /* execute all operations on the given issue */
    while (argc > 0) {
        const char *operation = shift(&argc, &argv);

        if (strcmp("comments", operation) == 0)
            ghcli_issue_comments(stdout, org, repo, issue);
        else if (strcmp("summary", operation) == 0)
            ghcli_issue_summary(stdout, org, repo, issue);
        else if (strcmp("close", operation) == 0)
            ghcli_issue_close(org, repo, issue);
        else if (strcmp("reopen", operation) == 0)
            ghcli_issue_reopen(org, repo, issue);
        else
            errx(1, "error: unknown operation %s", operation);
    }

    return EXIT_SUCCESS;
}

static int
subcommand_review(int argc, char *argv[])
{
    int         ch        = 0;
    int         pr        = -1;
    int         review_id = -1;
    const char *org       = NULL;
    const char *repo      = NULL;

    /* parse options */
    while ((ch = getopt(argc, argv, "p:o:r:c:")) != -1) {
        switch (ch) {
        case 'p': {
            char *endptr = NULL;
            pr = strtol(optarg, &endptr, 10);
            if (endptr != (optarg + strlen(optarg)))
                err(1, "error: cannot parse pr number");

            if (pr < 0)
                errx(1, "error: pr number is out of range");
        } break;
        case 'o':
            org = optarg;
            break;
        case 'r':
            repo = optarg;
            break;
        case 'c': {
            char *endptr;
            review_id = strtol(optarg, &endptr, 10);

            if (optarg + strlen(optarg) != endptr)
                err(1, "error: cannot parse comment id");
        } break;
        case '?':
        default:
            usage();
        }
    }

    argc -= optind;
    argv += optind;

    check_org_and_repo(&org, &repo);

    if (pr > 0 && review_id > 0) {
        /* print comments */
        ghcli_pr_review_comment *comments  = NULL;
        size_t                   comments_size =
            ghcli_review_get_review_comments(
                org, repo, pr, review_id, &comments);

        ghcli_review_print_comments(stdout, comments, comments_size);
        ghcli_review_comments_free(comments, comments_size);
        return 0;
    } else if (pr > 0) {
        /* list reviews */
        ghcli_pr_review *reviews      = NULL;
        size_t           reviews_size = ghcli_review_get_reviews(
            org, repo, pr, &reviews);
        ghcli_review_print_review_table(stdout, reviews, reviews_size);
        ghcli_review_reviews_free(reviews, reviews_size);
        return 0;
    } else {
        sn_unimplemented;
    }

    return 1;
}

static int
subcommand_forks_create(int argc, char *argv[])
{
    int ch;
    const char *org = NULL, *repo = NULL, *in = NULL;
    while ((ch = getopt(argc, argv, "o:r:i:")) != -1) {
        switch (ch) {
        case 'o':
            org = optarg;
            break;
        case 'r':
            repo = optarg;
            break;
        case 'i':
            in = optarg;
            break;
        case '?':
        default:
            usage();
        }
    }

    argc -= optind;
    argv += optind;

    check_org_and_repo(&org, &repo);

    ghcli_fork_create(org, repo, in);

    return EXIT_SUCCESS;
}

static void
delete_repo(bool always_yes, const char *org, const char *repo)
{
    bool delete = false;

    if (!always_yes) {
        delete = sn_yesno(
            "Are you sure you want to delete %s/%s?",
            org, repo);
    } else {
        delete = true;
    }

    if (delete)
        ghcli_repo_delete(org, repo);
    else
        errx(1, "Operation aborted");
}

static int
subcommand_repos(int argc, char *argv[])
{
    int         ch, repos_size;
    const char *org        = NULL;
    const char *repo       = NULL;
    ghcli_repo *repos      = NULL;
    bool        always_yes = false;

    while ((ch = getopt(argc, argv, "o:r:y")) != -1) {
        switch (ch) {
        case 'o':
            org = optarg;
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

    /* List repos of the user/org */
    if (argc == 0) {
        if (repo)
            errx(1, "repos: no actions specified");

        if (!org)
            repos_size = ghcli_get_own_repos(&repos);
        else
            repos_size = ghcli_get_repos(org, &repos);

        ghcli_print_repos_table(stdout, repos, (size_t)repos_size);
        ghcli_repos_free(repos, repos_size);
    } else {
        check_org_and_repo(&org, &repo);

        for (size_t i = 0; i < (size_t)argc; ++i) {
            const char *action = argv[i];

            if (strcmp(action, "delete") == 0) {
                delete_repo(always_yes, org, repo);
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

    while ((ch = getopt(argc, argv, "f:d:")) != -1) {
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

    while ((ch = getopt(argc, argv, "y")) != -1) {
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
    int         ch;
    const char *user       = NULL;
    ghcli_gist *gists      = NULL;
    int         gists_size = 0;

    for (size_t i = 0; i < ARRAY_SIZE(gist_subcommands); ++i) {
        if (argc > 1 && strcmp(argv[1], gist_subcommands[i].name) == 0) {
            argc -= 1;
            argv += 1;
            return gist_subcommands[i].fn(argc, argv);
        }
    }

    while ((ch = getopt(argc, argv, "u:")) != -1) {
        switch (ch) {
        case 'u':
            user = optarg;
            break;
        case '?':
        default:
            usage();
        }
    }

    argc -= optind;
    argv += optind;

    gists_size = ghcli_get_gists(user, &gists);
    ghcli_print_gists_table(stdout, gists, gists_size);
    return EXIT_SUCCESS;
}

static int
subcommand_forks(int argc, char *argv[])
{
    ghcli_fork *forks      = NULL;
    const char *org        = NULL, *repo = NULL;
    int         forks_size = 0;
    int         ch         = 0;
    bool        always_yes = false;

    /* detect whether we wanna create a fork */
    if (argc > 1 && (strcmp(argv[1], "create") == 0)) {
        shift(&argc, &argv);
        return subcommand_forks_create(argc, argv);
    }

    while ((ch = getopt(argc, argv, "o:r:y")) != -1) {
        switch (ch) {
        case 'o':
            org = optarg;
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

    check_org_and_repo(&org, &repo);

    if (argc == 0) {
        forks_size = ghcli_get_forks(org, repo, &forks);
        ghcli_print_forks(stdout, forks, forks_size);
        return EXIT_SUCCESS;
    }

    for (size_t i = 0; i < (size_t)argc; ++i) {
        const char *action = argv[i];

        if (strcmp(action, "delete") == 0) {
            delete_repo(always_yes, org, repo);
        } else {
            errx(1, "forks: unknown action '%s'", action);
        }
    }

    return EXIT_SUCCESS;
}

static int
subcommand_create_release(int argc, char *argv[])
{
    ghcli_new_release release     = {0};
    int               ch;

    const struct option options[] = {
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
        {0},
    };

    while ((ch = getopt_long(argc, argv, "dpn:t:c:r:o:",
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
        default:
            usage();
        }
    }

    argc -= optind;
    argv += optind;

    check_org_and_repo(&release.owner, &release.repo);

    if (!release.tag)
        errx(1, "releases create: missing tag name");

    if (!sn_yesno("Do you want to create this release?"))
        errx(1, "Aborted by user");

    return EXIT_SUCCESS;
}

static int
subcommand_releases(int argc, char *argv[])
{
    int            ch, releases_size;
    const char    *org      = NULL;
    const char    *repo     = NULL;
    ghcli_release *releases = NULL;

    if (argc > 1 && strcmp("create", argv[1]) == 0)
        return subcommand_create_release(argc - 1, argv + 1);

    while ((ch = getopt(argc, argv, "o:r:")) != -1) {
        switch (ch) {
        case 'o':
            org = optarg;
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

    check_org_and_repo(&org, &repo);

    releases_size = ghcli_get_releases(org, repo, &releases);
    ghcli_print_releases(stdout, releases, releases_size);
    ghcli_free_releases(releases, releases_size);

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
    { .cmd_name = "pulls",    .fn = subcommand_pulls    },
    { .cmd_name = "issues",   .fn = subcommand_issues   },
    { .cmd_name = "repos",    .fn = subcommand_repos    },
    { .cmd_name = "comment",  .fn = subcommand_comment  },
    { .cmd_name = "review",   .fn = subcommand_review   },
    { .cmd_name = "forks",    .fn = subcommand_forks    },
    { .cmd_name = "gists",    .fn = subcommand_gists    },
    { .cmd_name = "releases", .fn = subcommand_releases },
    { .cmd_name = "version",  .fn = subcommand_version  },
};

int
main(int argc, char *argv[])
{
    /* discard program name */
    shift(&argc, &argv);

    // TODO: accept arguments
    ghcli_config_init(NULL);

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
