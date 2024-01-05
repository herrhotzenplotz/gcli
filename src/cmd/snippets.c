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

#include <gcli/cmd/cmd.h>
#include <gcli/cmd/table.h>

#include <gcli/gitlab/snippets.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <stdlib.h>

static void
usage(void)
{
	fprintf(stderr, "usage: gcli snippets [-n number] [-sl]\n");
	fprintf(stderr, "       gcli snippets delete id\n");
	fprintf(stderr, "       gcli snippets get id\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "  -l              Print a long list instead of a short table\n");
	fprintf(stderr, "  -n number       Number of snippets to fetch\n");
	fprintf(stderr, "  -s              Sort the output in reverse order\n");
	fprintf(stderr, "  -u user         User for whom to list gists\n");
	fprintf(stderr, "\n");
	version();
	copyright();
}

static void
gcli_print_snippet(enum gcli_output_flags const flags,
                   gcli_gitlab_snippet const *const it)
{
	gcli_dict dict;

	(void) flags;

	dict = gcli_dict_begin();

	gcli_dict_add(dict,        "ID",     0, 0, "%"PRIid, it->id);
	gcli_dict_add_string(dict, "TITLE",  0, 0, it->title);
	gcli_dict_add_string(dict, "AUTHOR", 0, 0, it->author);
	gcli_dict_add_string(dict, "FILE",   0, 0, it->filename);
	gcli_dict_add_string(dict, "DATE",   0, 0, it->date);
	gcli_dict_add_string(dict, "VSBLTY", 0, 0, it->visibility);
	gcli_dict_add_string(dict, "URL",    0, 0, it->raw_url);

	gcli_dict_end(dict);
}

static void
gcli_print_snippets_long(enum gcli_output_flags const flags,
                         gcli_gitlab_snippet_list const *const list, int const max)
{
	int n;

	/* Determine number of items to print */
	if (max < 0 || (size_t)(max) > list->snippets_size)
		n = list->snippets_size;
	else
		n = max;

	if (flags & OUTPUT_SORTED) {
		for (int i = 0; i < n; ++i)
			gcli_print_snippet(flags, &list->snippets[n-i-1]);
	} else {
		for (int i = 0; i < n; ++i)
			gcli_print_snippet(flags, &list->snippets[i]);
	}
}

static void
gcli_print_snippets_short(enum gcli_output_flags const flags,
                          gcli_gitlab_snippet_list const *const list,
                          int const max)
{
	int n;
	gcli_tbl table;
	struct gcli_tblcoldef cols[] = {
		{ .name = "ID",         .type = GCLI_TBLCOLTYPE_ID,     .flags = GCLI_TBLCOL_JUSTIFYR },
		{ .name = "DATE",       .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "VISIBILITY", .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "AUTHOR",     .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "TITLE",      .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
	};

	/* Determine number of items to print */
	if (max < 0 || (size_t)(max) > list->snippets_size)
		n = list->snippets_size;
	else
		n = max;

	/* Fill table */
	table = gcli_tbl_begin(cols, ARRAY_SIZE(cols));
	if (!table)
		errx(1, "gcli: error: could not init table");

	if (flags & OUTPUT_SORTED) {
		for (int i = 0; i < n; ++i)
			gcli_tbl_add_row(table,
			                 list->snippets[n-i-1].id,
			                 list->snippets[n-i-1].date,
			                 list->snippets[n-i-1].visibility,
			                 list->snippets[n-i-1].author,
			                 list->snippets[n-i-1].title);
	} else {
		for (int i = 0; i < n; ++i)
			gcli_tbl_add_row(table,
			                 list->snippets[i].id,
			                 list->snippets[i].date,
			                 list->snippets[i].visibility,
			                 list->snippets[i].author,
			                 list->snippets[i].title);
	}

	gcli_tbl_end(table);
}

void
gcli_snippets_print(enum gcli_output_flags const flags,
                    gcli_gitlab_snippet_list const *const list, int const max)
{
	if (list->snippets_size == 0) {
		puts("No Snippets");
		return;
	}

	if (flags & OUTPUT_LONG)
		gcli_print_snippets_long(flags, list, max);
	else
		gcli_print_snippets_short(flags, list, max);
}

static int
subcommand_snippet_get(int argc, char *argv[])
{
	argc -= 1;
	argv += 1;

	if (!argc) {
		fprintf(stderr, "gcli: error: expected ID of snippet to fetch\n");
		usage();
		return EXIT_FAILURE;
	}

	char *snippet_id = shift(&argc, &argv);

	if (argc) {
		fprintf(stderr, "gcli: error: stray arguments\n");
		usage();
		return EXIT_FAILURE;
	}

	if (gcli_snippet_get(g_clictx, snippet_id, stdout) < 0)
		errx(1, "gcli: error: failed to fetch snippet contents: %s",
		     gcli_get_error(g_clictx));

	return EXIT_SUCCESS;
}

static int
subcommand_snippet_delete(int argc, char *argv[])
{
	argc -= 1;
	argv += 1;

	if (!argc) {
		fprintf(stderr, "gcli: error: expected ID of snippet to delete\n");
		usage();
		return EXIT_FAILURE;
	}

	char *snippet_id = shift(&argc, &argv);

	if (argc) {
		fprintf(stderr, "gcli: error: trailing options\n");
		usage();
		return EXIT_FAILURE;
	}

	if (gcli_snippet_delete(g_clictx, snippet_id) < 0)
		errx(1, "gcli: error: failed to delete snippet: %s",
		     gcli_get_error(g_clictx));

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
	int ch;
	gcli_gitlab_snippet_list list = {0};
	int count = 30;
	enum gcli_output_flags flags = 0;

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
		{ .name    = "long",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 'l' },
		{0},
	};

	while ((ch = getopt_long(argc, argv, "sn:l", options, NULL)) != -1) {
		switch (ch) {
		case 'n': {
			char *endptr = NULL;
			count = strtol(optarg, &endptr, 10);

			if (endptr != (optarg + strlen(optarg)))
				err(1, "gcli: error: cannot parse snippets count");

			if (count == 0)
				errx(1, "gcli: error: snippets count must not be zero");
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

	if (gcli_snippets_get(g_clictx, count, &list) < 0)
		errx(1, "gcli: error: failed to fetch snippets: %s",
		     gcli_get_error(g_clictx));

	gcli_snippets_print(flags, &list, count);
	gcli_snippets_free(&list);

	return EXIT_SUCCESS;
}
