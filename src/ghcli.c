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

#include <ghcli/issues.h>
#include <ghcli/pulls.h>

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
    ghcli_pull *pulls      = NULL;
    int         pulls_size = 0;

    if (argc == 2) {
        pulls_size = ghcli_get_pulls(argv[0], argv[1], &pulls);
        ghcli_print_pulls_table(stdout, pulls, pulls_size);

        return EXIT_SUCCESS;
    }

    int diff = 0;
    while (argc > 2) {
        const char *option = shift(&argc, &argv);

        if (strcmp(option, "--diff") == 0) {

            if (diff)
                errx(1, "--diff is specified multiple times");

            char *optarg = shift(&argc, &argv);
            char *endptr = NULL;

            diff = strtoul(optarg, &endptr, 10);
            if (endptr != (optarg + strlen(optarg)))
                err(1, "cannot parse pr number of --diff option");

            if (diff <= 0)
                errx(1, "pr number is out of range");
        } else {
            errx(1, "unknown option %s", option);
        }
    }

    ghcli_print_pull_diff(stdout, argv[0], argv[1], diff);

    return EXIT_SUCCESS;
}

static int
subcommand_issues(int argc, char *argv[])
{
    ghcli_issue *issues      = NULL;
    int          issues_size = 0;

    (void) argc;

    issues_size = ghcli_get_issues(argv[0], argv[1], &issues);

    ghcli_print_issues_table(stdout, issues, issues_size);

    return EXIT_SUCCESS;
}

int
main(int argc, char *argv[])
{
    shift(&argc, &argv);

    const char *subcommand = shift(&argc, &argv);

    if (strcmp(subcommand, "pulls") == 0)
        return subcommand_pulls(argc, argv);
    else if (strcmp(subcommand, "issues") == 0)
        return subcommand_issues(argc, argv);
    else
        errx(1, "unknown subcommand %s", subcommand);

    return 42;
}
