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
