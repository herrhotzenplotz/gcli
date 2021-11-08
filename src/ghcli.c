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
#include <ghcli/gitconfig.h>
#include <ghcli/issues.h>
#include <ghcli/pulls.h>
#include <ghcli/review.h>

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
    fprintf(stderr,
            "ghcli version "GHCLI_VERSION_STRING" - a command line utility to interact with GitHub\n"
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

static sn_sv
pr_try_derive_head(void)
{
    sn_sv account = {0};
    sn_sv branch  = {0};

    if (!(account = ghcli_config_get_account()).length)
        errx(1,
             "error: Cannot derive PR head. Please specify --from or set the "
             "       account in the users ghcli config file.");

    if (!(branch = ghcli_gitconfig_get_current_branch()).length)
        errx(1,
             "error: Cannot derive PR head. Please specify --from or, if you are "
             "       in »detached HEAD« state, checkout the branch you want to "
             "       pull request.");

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
        { .name = "in", .has_arg = required_argument, .flag = NULL, .val = 'i' },
        {0},
    };

    while ((ch = getopt_long(argc, argv, "i:", options, NULL)) != -1) {
        switch (ch) {
        case 'i':
            opts.in    = SV(optarg);
            break;
        default:
            usage();
        }
    }

    argc -= optind;
    argv += optind;

    if (!opts.in.length) {
        if (!(opts.in = ghcli_config_get_upstream()).length)
            errx(1, "error: Target repo for the issue to be created in is missing. Please either specify '--in org/repo' or set pr.upstream in .ghcli.");
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
        { .name = "from",  .has_arg = required_argument, .flag = NULL,        .val = 'f' },
        { .name = "to",    .has_arg = required_argument, .flag = NULL,        .val = 't' },
        { .name = "in",    .has_arg = required_argument, .flag = NULL,        .val = 'i' },
        { .name = "draft", .has_arg = no_argument,       .flag = &opts.draft, .val = 1   },
        {0},
    };

    while ((ch = getopt_long(argc, argv, "f:t:di:", options, NULL)) != -1) {
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
            errx(1, "error: PR base is missing. Please either specify --to branch-name or set pr.base in .ghcli.");
    }

    if (!opts.in.length) {
        if (!(opts.in = ghcli_config_get_upstream()).length)
            errx(1, "error: PR target repo is missing. Please either specify --in org/repo or set pr.upstream in .ghcli.");
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

    const struct option options[] = {
        { .name = "repo",  .has_arg = required_argument, .flag = NULL, .val = 'r' },
        { .name = "org",   .has_arg = required_argument, .flag = NULL, .val = 'o' },
        { .name = "issue", .has_arg = required_argument, .flag = NULL, .val = 'i' },
        { .name = "pull",  .has_arg = required_argument, .flag = NULL, .val = 'p' },
        {0},
    };

    while ((ch = getopt_long(argc, argv, "r:o:i:p:", options, NULL)) != -1) {
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
        default:
            usage();
        }
    }

    argc -= optind;
    argv += optind;

    if ((org == NULL) != (repo == NULL))
        errx(1, "error: missing either explicit org or repo");

    if (org == NULL)
        ghcli_gitconfig_get_repo(&org, &repo);

    if (issue < 0)
        errx(1, "error: missing issue/PR number (use -i)");

    ghcli_comment_submit((ghcli_submit_comment_opts) { .org = org, .repo = repo, .issue = issue });

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

    /* If unset try to autodetect github remote */
    if ((org == NULL) != (repo == NULL))
        errx(1, "error: missing either explicit org or repo");

    if (org == NULL)
        ghcli_gitconfig_get_repo(&org, &repo);

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

    /* If no remote was specified, try to autodetect */
    if ((org == NULL) != (repo == NULL))
        errx(1, "error: missing either explicit org or repo");

    if (org == NULL)
        ghcli_gitconfig_get_repo(&org, &repo);

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

    /* If no remote was specified, try to autodetect */
    if ((org == NULL) != (repo == NULL))
        errx(1, "error: missing either explicit org or repo");

    if (org == NULL)
        ghcli_gitconfig_get_repo(&org, &repo);

    if (pr > 0 && review_id > 0) {
        /* print comments */
        ghcli_pr_review_comment *comments  = NULL;
        size_t                   comments_size =
            ghcli_review_get_review_comments(org, repo, pr, review_id, &comments);

        ghcli_review_print_comments(stdout, comments, comments_size);
        ghcli_review_comments_free(comments, comments_size);
        return 0;
    } else if (pr > 0) {
        /* list reviews */
        ghcli_pr_review *reviews      = NULL;
        size_t           reviews_size = ghcli_review_get_reviews(org, repo, pr, &reviews);
        ghcli_review_print_review_table(stdout, reviews, reviews_size);
        ghcli_review_reviews_free(reviews, reviews_size);
        return 0;
    } else {
        sn_unimplemented;
    }

    return 1;
}

int
main(int argc, char *argv[])
{
    /* discard program name */
    shift(&argc, &argv);

    // TODO: accept arguments
    ghcli_config_init(NULL);

    if (argc == 0)
        errx(1, "error: missing subcommand");

    if (strcmp(argv[0], "pulls") == 0) {
        return subcommand_pulls(argc, argv);
    } else if (strcmp(argv[0], "issues") == 0) {
        return subcommand_issues(argc, argv);
    } else if (strcmp(argv[0], "comment") == 0) {
        return subcommand_comment(argc, argv);
    } else if (strcmp(argv[0], "review") == 0) {
        return subcommand_review(argc, argv);
    } else if (strcmp(argv[0], "version") == 0) {
        version();
        return 0;
    } else {
        fprintf(stderr, "error: unknown subcommand %s\n", argv[0]);
        usage();
    }

    return 42;
}
