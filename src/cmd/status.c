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
#include <gcli/status.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

static void
usage(void)
{
    fprintf(stderr, "usage: gcli status -m id\n");
    fprintf(stderr, "       gcli status [-n number]\n");
    fprintf(stderr, "OPTIONS:\n");
    fprintf(stderr, "  -n number       Number of messages to fetch\n");
    fprintf(stderr, "  -m id           Mark the given message as read\n");
    fprintf(stderr, "\n");
    version();
    copyright();
}

int
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
            return EXIT_FAILURE;
        }
    }

    argc -= optind;
    argv += optind;

    if (!mark) {
        gcli_status(count);
    } else {
        if (count != 30)
            warnx("ignoring -n/--count argument");

        if (argc > 1) {
            fprintf(stderr, "error: too many arguments for marking notifications\n");
            usage();
            return EXIT_FAILURE;
        }

        if (argc < 1) {
            fprintf(stderr, "error: missing notification id to mark as read\n");
            usage();
            return EXIT_FAILURE;
        }

        gcli_notification_mark_as_read(argv[0]);
    }

    return EXIT_SUCCESS;
}
