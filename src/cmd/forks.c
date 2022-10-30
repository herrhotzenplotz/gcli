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

#include "config.h"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <stdlib.h>

#include <gcli/cmd.h>
#include <gcli/config.h>
#include <gcli/gitconfig.h>
#include <gcli/forks.h>

static void
usage(void)
{
    fprintf(stderr, "usage: gcli forks create [-o owner -r repo] [-i target] [-y]\n");
    fprintf(stderr, "       gcli forks [-o owner -r repo] [-n number] [-s] [-y] [delete]\n");
    fprintf(stderr, "OPTIONS:\n");
    fprintf(stderr, "  -o owner        The repository owner\n");
    fprintf(stderr, "  -r repo         The repository name\n");
    fprintf(stderr, "  -i target       Name of org or user to create the fork in\n");
    fprintf(stderr, "  -n number       Number of forks to fetch (-1 = everything)\n");
    fprintf(stderr, "  -s              Print (sort) in reverse order\n");
    fprintf(stderr, "  -y              Do not ask for confirmation\n");
    fprintf(stderr, "\n");
    version();
    copyright();
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
            return EXIT_FAILURE;
        }
    }

    argc -= optind;
    argv += optind;

    check_owner_and_repo(&owner, &repo);

    gcli_fork_create(owner, repo, in);

    if (!always_yes) {
        if (!sn_yesno("Do you want to add a remote for the fork?"))
            return EXIT_SUCCESS;
    }

    if (!in)
        in = sn_sv_to_cstr(gcli_config_get_account());

    gcli_gitconfig_add_fork_remote(in, repo);

    return EXIT_SUCCESS;
}

int
subcommand_forks(int argc, char *argv[])
{
    gcli_fork              *forks      = NULL;
    const char             *owner      = NULL, *repo = NULL;
    int                     forks_size = 0;
    int                     ch         = 0;
    int                     count      = 30;
    bool                    always_yes = false;
    enum gcli_output_flags  flags      = 0;

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
            flags |= OUTPUT_SORTED;
            break;
        case '?':
        default:
            usage();
            return EXIT_FAILURE;
        }
    }

    argc -= optind;
    argv += optind;

    check_owner_and_repo(&owner, &repo);

    if (argc == 0) {
        forks_size = gcli_get_forks(owner, repo, count, &forks);
        gcli_print_forks(flags, forks, forks_size);
        return EXIT_SUCCESS;
    }

    for (size_t i = 0; i < (size_t)argc; ++i) {
        const char *action = argv[i];

        if (strcmp(action, "delete") == 0) {
            delete_repo(always_yes, owner, repo);
        } else {
            fprintf(stderr, "error: forks: unknown action '%s'\n", action);
        }
    }

    return EXIT_SUCCESS;
}
