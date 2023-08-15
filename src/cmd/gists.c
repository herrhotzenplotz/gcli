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

#include <config.h>

#include <gcli/cmd.h>
#include <gcli/config.h>
#include <gcli/curl.h>
#include <gcli/github/gists.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <stdlib.h>
#include <unistd.h>

static void
usage(void)
{
	fprintf(stderr, "usage: gcli gists [-u user] [-n number] [-s]\n");
	fprintf(stderr, "       gcli gists create [-d description] [-f file] name\n");
	fprintf(stderr, "       gcli gists delete [-y] id\n");
	fprintf(stderr, "       gcli gists get id name\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "  -d description  Description of the gist\n");
	fprintf(stderr, "  -f file         Path to file to upload (otherwise stdin)\n");
	fprintf(stderr, "  -l              Print a long list instead of a short table\n");
	fprintf(stderr, "  -n number       Number of gists to fetch\n");
	fprintf(stderr, "  -s              Print (sort) in reverse order\n");
	fprintf(stderr, "  -u user         User for whom to list gists\n");
	fprintf(stderr, "  -y              Do not ask for confirmation\n");
	fprintf(stderr, "\n");
	version();
	copyright();
}

static int
subcommand_gist_get(int argc, char *argv[])
{
	shift(&argc, &argv); /* Discard the *get* */

	char const *gist_id = shift(&argc, &argv);
	char const *file_name = shift(&argc, &argv);
	gcli_gist *gist = NULL;
	gcli_gist_file *file = NULL;

	gist = gcli_get_gist(g_clictx, gist_id);

	for (size_t f = 0; f < gist->files_size; ++f) {
		if (sn_sv_eq_to(gist->files[f].filename, file_name)) {
			file = &gist->files[f];
			goto file_found;
		}
	}

	errx(1, "gists get: %s: no such file in gist with id %s",
	     file_name, gist_id);

file_found:

	if (isatty(STDOUT_FILENO) && (file->size >= 4 * 1024 * 1024))
		errx(1, "File is bigger than 4 MiB, refusing to print to stdout.");

	if (gcli_curl(g_clictx, stdout, file->url.data, file->type.data) < 0)
		errx(1, "error: failed to fetch gist");

	return EXIT_SUCCESS;
}

static int
subcommand_gist_create(int argc, char *argv[])
{
	int            ch;
	gcli_new_gist  opts = {0};
	char const    *file = NULL;

	struct option const options[] = {
		{ .name    = "file",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'r' },
		{ .name    = "description",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'd' },
		{0},
	};

	while ((ch = getopt_long(argc, argv, "f:d:", options, NULL)) != -1) {
		switch (ch) {
		case 'f':
			file = optarg;
			break;
		case 'd':
			opts.gist_description = optarg;
			break;
		case '?':
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc != 1) {
		fprintf(stderr, "error: gists create: missing file name for gist\n");
		usage();
		return EXIT_FAILURE;
	}

	opts.file_name = shift(&argc, &argv);

	if (file) {
		if ((opts.file = fopen(file, "r")) == NULL)
			err(1, "gists create: cannot open file");
	} else {
		opts.file = stdin;
	}

	if (!opts.gist_description)
		opts.gist_description = "gcli paste";

	if (gcli_create_gist(g_clictx, opts) < 0)
		errx(1, "error: failed to create gist");

	return EXIT_SUCCESS;
}

static int
subcommand_gist_delete(int argc, char *argv[])
{
	int         ch;
	bool        always_yes = false;
	char const *gist_id    = NULL;

	struct option const options[] = {
		{ .name    = "yes",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 'y' },
		{0},
	};

	while ((ch = getopt_long(argc, argv, "y", options, NULL)) != -1) {
		switch (ch) {
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

	gist_id = shift(&argc, &argv);
	gcli_delete_gist(g_clictx, gist_id, always_yes);

	return EXIT_SUCCESS;
}

static struct {
	char const *name;
	int (*fn)(int, char **);
} gist_subcommands[] = {
	{ .name = "get",    .fn = subcommand_gist_get    },
	{ .name = "create", .fn = subcommand_gist_create },
	{ .name = "delete", .fn = subcommand_gist_delete },
};

int
subcommand_gists(int argc, char *argv[])
{
	int                     ch;
	char const             *user  = NULL;
	gcli_gist_list          gists = {0};
	int                     count = 30;
	enum gcli_output_flags  flags = 0;

	/* Make sure we are looking at a GitHub forge */
	if (gcli_config_get_forge_type(g_clictx) != GCLI_FORGE_GITHUB) {
		errx(1, "error: The gists subcommand only works for Github "
		     "forges. Please use either -a or -t to force using a "
		     "Github account.");
	}

	for (size_t i = 0; i < ARRAY_SIZE(gist_subcommands); ++i) {
		if (argc > 1 && strcmp(argv[1], gist_subcommands[i].name) == 0) {
			argc -= 1;
			argv += 1;
			return gist_subcommands[i].fn(argc, argv);
		}
	}

	struct option const options[] = {
		{ .name    = "user",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'u' },
		{ .name    = "count",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'n' },
		{ .name    = "sorted",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 's' },
		{ .name    = "long",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 'l' },
		{0},
	};

	while ((ch = getopt_long(argc, argv, "lsn:u:", options, NULL)) != -1) {
		switch (ch) {
		case 'u':
			user = optarg;
			break;
		case 'n': {
			char *endptr = NULL;
			count        = strtol(optarg, &endptr, 10);
			if (endptr != (optarg + strlen(optarg)))
				err(1, "gists: cannot parse gists count");
		} break;
		case 's':
			flags |= OUTPUT_SORTED;
			break;
		case 'l':
			flags |= OUTPUT_LONG;
			break;
		case '?':
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	if (gcli_get_gists(g_clictx, user, count, &gists) < 0)
		errx(1, "error: failed to get gists");

	gcli_print_gists(g_clictx, flags, &gists, count);
	gcli_gists_free(&gists);

	return EXIT_SUCCESS;
}
