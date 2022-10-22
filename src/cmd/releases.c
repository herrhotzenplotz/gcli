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

#include <gcli/cmd.h>
#include <gcli/editor.h>
#include <gcli/releases.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <stdlib.h>

static void
usage(void)
{
    fprintf(stderr, "usage: gcli releases create [-o owner -r repo] [-n name] "
            "[-y] [-d] [-p] [-a asset]\n");
    fprintf(stderr, "                            [-c commitish] [-t tag]\n");
    fprintf(stderr, "       gcli releases delete [-o owner -r repo] [-y] id\n");
    fprintf(stderr, "       gcli releases [-o owner -r repo] [-n number] [-s]\n");
    fprintf(stderr, "OPTIONS:\n");
    fprintf(stderr, "  -o owner        The repository owner\n");
    fprintf(stderr, "  -r repo         The repository name\n");
    fprintf(stderr, "  -a asset        Path to file to upload as release asset\n");
    fprintf(stderr, "  -c committish   A ref/commit/branch that the release is created from\n");
    fprintf(stderr, "  -d              Mark as a release draft\n");
    fprintf(stderr, "  -n name         Name of the created release\n");
    fprintf(stderr, "  -n number       Number of releases to fetch (-1 = everything)\n");
    fprintf(stderr, "  -p              Mark as a prerelease\n");
    fprintf(stderr, "  -t tag          Name for new tag\n");
    fprintf(stderr, "  -y              Do not ask for confirmation\n");
    fprintf(stderr, "\n");
    version();
    copyright();
}

static void
releasemsg_init(FILE *f, void *_data)
{
    const gcli_new_release *info = _data;

    fprintf(
        f,
        "! Enter your release notes above, save and exit.\n"
        "! All lines with a leading '!' are discarded and will not\n"
        "! appear in the final release note.\n"
        "!       IN : %s/%s\n"
        "! TAG NAME : %s\n"
        "!     NAME : %s\n",
        info->owner, info->repo, info->tag, info->name);
}

static sn_sv
get_release_message(const gcli_new_release *info)
{
    return gcli_editor_get_user_message(releasemsg_init, (void *)info);
}

static int
subcommand_releases_create(int argc, char *argv[])
{
    gcli_new_release release    = {0};
    int              ch;
    bool             always_yes = false;

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
            gcli_release_asset asset = {
                .path  = optarg,
                .name  = optarg,
                .label = "unused",
            };
            gcli_release_push_asset(&release, asset);
        } break;
        case 'y': {
            always_yes = true;
        } break;
        default:
            usage();
            return EXIT_FAILURE;
        }
    }

    argc -= optind;
    argv += optind;

    check_owner_and_repo(&release.owner, &release.repo);

    /* make sure we have a tag for the release */
    if (!release.tag) {
        fprintf(stderr, "error: releases create: missing tag name\n");
        usage();
        return EXIT_FAILURE;
    }

    release.body = get_release_message(&release);

    if (!always_yes)
        if (!sn_yesno("Do you want to create this release?"))
            errx(1, "Aborted by user");

    gcli_create_release(&release);

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
            return EXIT_FAILURE;
        }
    }

    argc -= optind;
    argv += optind;

    check_owner_and_repo(&owner, &repo);

    /* make sure the user supplied the release id */
    if (argc != 1) {
        fprintf(stderr, "error: releases delete: missing release id\n");
        usage();
        return EXIT_FAILURE;
    }

    if (!always_yes)
        if (!sn_yesno("Are you sure you want to delete this release?"))
            errx(1, "Aborted by user");

    gcli_delete_release(owner, repo, argv[0]);

    return EXIT_SUCCESS;
}

static struct {
    const char *name;
    int (*fn)(int, char **);
} releases_subcommands[] = {
    { .name = "delete", .fn = subcommand_releases_delete },
    { .name = "create", .fn = subcommand_releases_create },
};

int
subcommand_releases(int argc, char *argv[])
{
    int                     ch;
    int                     releases_size;
    int                     count    = 30;
    const char             *owner    = NULL;
    const char             *repo     = NULL;
    gcli_release           *releases = NULL;
    enum gcli_output_order  order    = OUTPUT_ORDER_UNSORTED;

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
            return EXIT_FAILURE;
        }
    }

    argc -= optind;
    argv += optind;

    /* sanity check */
    if (argc > 0) {
        fprintf(stderr, "error: stray arguments\n");
        usage();
        return EXIT_FAILURE;
    }

    check_owner_and_repo(&owner, &repo);

    releases_size = gcli_get_releases(owner, repo, count, &releases);
    gcli_print_releases(order, releases, releases_size);
    gcli_free_releases(releases, releases_size);

    return EXIT_SUCCESS;
}
