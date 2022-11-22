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
#include <gcli/labels.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <stdlib.h>
#include <string.h>

static void
usage(void)
{
	fprintf(stderr, "usage: gcli labels create [-o owner -r repo] -n name -c color -d description\n");
	fprintf(stderr, "       gcli labels delete [-o owner -r repo] id\n");
	fprintf(stderr, "       gcli labels [-o owner -r repo] [-n number]\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "  -o owner        The repository owner\n");
	fprintf(stderr, "  -r repo         The repository name\n");
	fprintf(stderr, "  -n number       Number of labels to fetch (-1 = everything)\n");
	fprintf(stderr, "  -l name         Name of the new label\n");
	fprintf(stderr, "  -c color        Six digit hex code of the label's color\n");
	fprintf(stderr, "  -d description  A short description of the label\n");
	fprintf(stderr, "\n");
	version();
	copyright();
}

static int
subcommand_labels_delete(int argc, char *argv[])
{
	int         ch;
	char const *owner = NULL, *repo = NULL;
	const struct option options[] = {
		{.name = "repo",  .has_arg = required_argument, .val = 'r'},
		{.name = "owner", .has_arg = required_argument, .val = 'o'},
		{0},
	};

	while ((ch = getopt_long(argc, argv, "o:r:", options, NULL)) != -1) {
		switch (ch) {
		case 'o':
			owner = optarg;
			break;
		case 'r':
			repo = optarg;
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

	if (argc != 1) {
		fprintf(stderr, "error: missing label to delete\n");
		usage();
		return EXIT_FAILURE;
	}

	gcli_delete_label(owner, repo, argv[0]);

	return EXIT_SUCCESS;
}

static int
subcommand_labels_create(int argc, char *argv[])
{
	gcli_label  label = {0};
	char const *owner = NULL, *repo = NULL;
	int         ch;

	const struct option options[] = {
		{.name = "repo",        .has_arg = required_argument, .val = 'r'},
		{.name = "owner",       .has_arg = required_argument, .val = 'o'},
		{.name = "name",        .has_arg = required_argument, .val = 'n'},
		{.name = "color",       .has_arg = required_argument, .val = 'c'},
		{.name = "description", .has_arg = required_argument, .val = 'd'},
		{0}
	};

	while ((ch = getopt_long(argc, argv, "n:o:r:d:c:", options, NULL)) != -1) {
		switch (ch) {
		case 'o':
			owner = optarg;
			break;
		case 'r':
			repo = optarg;
			break;
		case 'c': {
			char *endptr = NULL;
			label.color = strtol(optarg, &endptr, 16);
			if (endptr != (optarg + strlen(optarg)))
				err(1, "labels: cannot parse color");
		} break;
		case 'd': {
			label.description = optarg;
		} break;
		case 'n': {
			label.name = optarg;
		} break;
		case '?':
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	check_owner_and_repo(&owner, &repo);

	if (!label.name) {
		fprintf(stderr, "error: missing name for label\n");
		usage();
		return EXIT_FAILURE;
	}

	if (!label.description) {
		fprintf(stderr, "error: missing description for label\n");
		usage();
		return EXIT_FAILURE;
	}

	gcli_create_label(owner, repo, &label);

	/* only if we are not quieted */
	if (!sn_quiet())
		gcli_print_labels(&label, 1);

	gcli_free_label(&label);

	return EXIT_SUCCESS;
}

static struct {
	char const *name;
	int (*fn)(int, char **);
} labels_subcommands[] = {
	{ .name = "delete", .fn = subcommand_labels_delete },
	{ .name = "create", .fn = subcommand_labels_create },
};

int
subcommand_labels(int argc, char *argv[])
{
	int         count = 30;
	int         ch;
	char const *owner = NULL, *repo = NULL;
	size_t      labels_count;
	gcli_label *labels;

	const struct option options[] = {
		{.name = "repo",  .has_arg = required_argument, .flag = NULL, .val = 'r'},
		{.name = "owner", .has_arg = required_argument, .flag = NULL, .val = 'o'},
		{.name = "count", .has_arg = required_argument, .flag = NULL, .val = 'n'},
		{0}
	};

	if (argc > 1) {
		for (size_t i = 0; i < ARRAY_SIZE(labels_subcommands); ++i) {
			if (strcmp(labels_subcommands[i].name, argv[1]) == 0)
				return labels_subcommands[i].fn(argc - 1, argv + 1);
		}
	}

	while ((ch = getopt_long(argc, argv, "n:o:r:", options, NULL)) != -1) {
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
				err(1, "labels: cannot parse label count");
		} break;
		case '?':
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	/* sanity check: we must have parsed everything by now */
	if (argc > 0) {
		fprintf(stderr, "error: stray arguments\n");
		usage();
		return EXIT_FAILURE;
	}

	check_owner_and_repo(&owner, &repo);

	labels_count = gcli_get_labels(owner, repo, count, &labels);

	gcli_print_labels(labels, labels_count);
	gcli_free_labels(labels, labels_count);

	return EXIT_SUCCESS;
}
