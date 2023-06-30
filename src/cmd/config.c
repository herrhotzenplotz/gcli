/*
 * Copyright 2023 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gcli/cmd.h>
#include <gcli/config.h>
#include <gcli/curl.h>

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

static void
usage(void)
{
	fprintf(stderr, "usage: gcli config ssh\n");
	fprintf(stderr, "\n");
	version();
	copyright();
}

static int
subcommand_ssh(int argc, char *argv[])
{
	(void) argc;
	(void) argv;

	return 0;
}

struct subcommand {
	char const *const name;
	int (*fn)(int argc, char *argv[]);
} subcommands[] = {
	{ .name = "ssh", .fn = subcommand_ssh },
};

int
subcommand_config(int argc, char *argv[])
{
	int ch;

	struct option options[] = {
		{0}
	};

	while ((ch = getopt_long(argc, argv, "+", options, NULL)) != -1) {
		switch (ch) {
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	/* Check if the user gave us at least one option for this
	 * subcommand */
	if (argc == 0) {
		fprintf(stderr, "error: missing subcommand for config\n");
		usage();
		return EXIT_FAILURE;
	}

	for (size_t i = 0; i < ARRAY_SIZE(subcommands); ++i) {
		if (strcmp(argv[0], subcommands[i].name) == 0)
			return subcommands[i].fn(argc, argv);
	}

	fprintf(stderr, "error: unrecognised config subcommand »%s«\n",
	        argv[0]);
	usage();

	return EXIT_FAILURE;
}
