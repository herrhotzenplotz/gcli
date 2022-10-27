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
#include <gcli/snippets.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <stdlib.h>

static void
usage(void)
{
    fprintf(stderr, "usage: gcli snippets [-n number] [-s]\n");
    fprintf(stderr, "       gcli snippets delete id\n");
    fprintf(stderr, "       gcli snippets get id\n");
    fprintf(stderr, "OPTIONS:\n");
    fprintf(stderr, "  -n number       Number of snippets to fetch\n");
    fprintf(stderr, "  -u user         User for whom to list gists\n");
    fprintf(stderr, "\n");
    version();
    copyright();
}

static int
subcommand_snippet_get(int argc, char *argv[])
{
    argc -= 1;
    argv += 1;

    if (!argc) {
        fprintf(stderr, "error: get snippets: expected ID of snippet to fetch\n");
        usage();
        return EXIT_FAILURE;
    }

    char *snippet_id = shift(&argc, &argv);

    if (argc) {
        fprintf(stderr, "error: stray arguments\n");
        usage();
        return EXIT_FAILURE;
    }

    gcli_snippet_get(snippet_id);

    return EXIT_SUCCESS;
}

static int
subcommand_snippet_delete(int argc, char *argv[])
{
    argc -= 1;
    argv += 1;

    if (!argc) {
        fprintf(stderr, "error: delete snippets: expected ID of snippet to delete\n");
        usage();
        return EXIT_FAILURE;
    }

    char *snippet_id = shift(&argc, &argv);

    if (argc) {
        fprintf(stderr, "error: delete snippet: trailing options\n");
        usage();
        return EXIT_FAILURE;
    }

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

int
subcommand_snippets(int argc, char *argv[])
{
    int                     ch;
    gcli_snippet           *snippets      = NULL;
    int                     snippets_size = 0;
    int                     count         = 30;
    enum gcli_output_flags  flags         = OUTPUT_LONG;

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

    snippets_size = gcli_snippets_get(count, &snippets);
    gcli_snippets_print(flags, snippets, snippets_size);

    return EXIT_SUCCESS;
}
