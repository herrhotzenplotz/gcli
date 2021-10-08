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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ghcli/issues.h>
#include <ghcli/pulls.h>
#include <ghcli/comments.h>
#include <ghcli/gitconfig.h>

#include <sn/sn.h>

static char *
shift(int *argc, char ***argv)
{
    if (*argc == 0)
        errx(1, "Not enough arguments");

    (*argc)--;
    return *((*argv)++);
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
                err(1, "cannot parse pr number »%s«", optarg);

            if (pr <= 0)
                errx(1, "pr number is out of range");
        } break;
        case 'a': {
            all = true;
        } break;
        case '?':
        default:
            errx(1, "RTFM");
        }
    }

    argc -= optind;
    argv += optind;

    /* If unset try to autodetect github remote */
    if ((org == NULL) != (repo == NULL))
        errx(1, "missing either explicit org or repo");

    if (org == NULL) {
        const char *path = ghcli_find_gitconfig();
        ghcli_gitconfig_get_repo(path, &org, &repo);
    }

    /* In case no explicit PR number was specified, list all
     * open PRs and exit */
    if (pr < 0) {
        pulls_size = ghcli_get_prs(org, repo, all, &pulls);
        ghcli_print_pr_table(stdout, pulls, pulls_size);

        return EXIT_SUCCESS;
    }

    /* If a PR number was given, require -a to be unset */
    if (all)
        errx(1, "-a cannot be combined with operations on a PR");

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
        else
            errx(1, "unknown operation %s", operation);
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
                err(1, "cannot parse issue number");

            if (issue < 0)
                errx(1, "issue number is out of range");
        } break;
        case 'a':
            all = true;
            break;
        case '?':
        default:
            errx(1, "RTFM");
        }
    }

    argc -= optind;
    argv += optind;

    /* If no remote was specified, try to autodetect */
    if ((org == NULL) != (repo == NULL))
        errx(1, "missing either explicit org or repo");

    if (org == NULL) {
        const char *path = ghcli_find_gitconfig();
        ghcli_gitconfig_get_repo(path, &org, &repo);
    }

    /* No issue number was given, so list all open issues */
    if (issue < 0) {
        issues_size = ghcli_get_issues(org, repo, all, &issues);
        ghcli_print_issues_table(stdout, issues, issues_size);
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
        else
            errx(1, "unknown operation %s", operation);
    }

    return EXIT_SUCCESS;
}

int
main(int argc, char *argv[])
{
    /* discard program name */
    shift(&argc, &argv);

    if (argc == 0)
        errx(1, "missing subcommand");

    if (strcmp(argv[0], "pulls") == 0)
        return subcommand_pulls(argc, argv);
    else if (strcmp(argv[0], "issues") == 0)
        return subcommand_issues(argc, argv);
    else
        errx(1, "unknown subcommand %s", argv[0]);

    return 42;
}
