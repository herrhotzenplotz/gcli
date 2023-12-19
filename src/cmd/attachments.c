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

#include <gcli/gcli.h>
#include <gcli/cmd/cmd.h>

#include <errno.h>
#include <getopt.h>
#include <stdlib.h>

static void
usage(void)
{
	fprintf(stderr, "usage: gcli [options] attachments -i <id> actions...\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "  -i id       Execute the given actions for the specified attachment id.\n");
	fprintf(stderr, "ACTIONS:\n");
	fprintf(stderr, "  get [-o path]  Fetch and dump the contents of the "
	                  "attachments to the given path or stdout\n");
	fprintf(stderr, "\n");
	version();
}

static int
action_attachment_get(int *argc, char ***argv, gcli_id const id)
{
	int ch;
	FILE *outfile = NULL;
	struct option options[] = {
		{ .name = "output", .has_arg = required_argument, .flag = NULL, .val = 'o' },
		{0},
	};

	while ((ch = getopt_long(*argc, *argv, "+o:", options, NULL)) != -1) {
		switch (ch) {
		case 'o': {
			outfile = fopen(optarg, "w");
			if (!outfile) {
				fprintf(stderr, "gcli: failed to open »%s«: %s\n",
				        optarg, strerror(errno));
				return EXIT_FAILURE;
			}
		} break;
		default: {
			usage();
			return EXIT_FAILURE;
		} break;
		}
	}

	*argc -= optind;
	*argv += optind;
	optind = 0; /* reset */

	/* -o wasn't specified */
	if (outfile == NULL)
		outfile = stdout;

	(void) id;

	fprintf(stderr, "gcli: get action is not yet implemented\n");
	return EXIT_FAILURE;
}

static struct action {
	char const *const name;
	int (*fn)(int *argc, char ***argv, gcli_id const id);
} const actions[] = {
	{ .name = "get", .fn = action_attachment_get },
};

static size_t const actions_size = ARRAY_SIZE(actions);

static struct action const *
find_action(char const *const name)
{
	for (size_t i = 0; i < actions_size; ++i) {
		if (strcmp(name, actions[i].name) == 0)
			return &actions[i];
	}
	return NULL;
}

int
subcommand_attachments(int argc, char *argv[])
{
	int ch;
	gcli_id iflag;
	bool iflag_seen = false;

	struct option options[] = {
		{ .name = "id", .has_arg = required_argument, .flag = NULL, .val = 'i' },
		{0},
	};

	while ((ch = getopt_long(argc, argv, "+i:", options, NULL)) != -1) {
		switch (ch) {
		case 'i': {
			char *endptr;

			iflag_seen = true;
			iflag = strtoull(optarg, &endptr, 10);

			if (optarg + strlen(optarg) != endptr) {
				fprintf(stderr, "gcli: bad attachment id »%s«\n", optarg);
				return EXIT_FAILURE;
			}
		} break;
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	optind = 0;  /* reset */

	if (!iflag_seen) {
		fprintf(stderr, "gcli: missing -i flag\n");
		usage();
		return EXIT_FAILURE;
	}

	if (argc == 0) {
		fprintf(stderr, "gcli: missing actions\n");
		usage();
		return EXIT_FAILURE;
	}

	while (argc) {
		int rc;
		char const *const action_name = *argv;
		struct action const *const action = find_action(action_name);

		if (action == NULL) {
			fprintf(stderr, "gcli: %s: no such action\n", action_name);
			usage();
			return EXIT_FAILURE;
		}

		rc = action->fn(&argc, &argv, iflag);
		if (rc)
			return rc;
	}

	return 0;
}
