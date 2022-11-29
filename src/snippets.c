/*
 * Copyright 2021, 2022 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/curl.h>
#include <gcli/gitlab/config.h>
#include <gcli/json_util.h>
#include <gcli/snippets.h>
#include <gcli/table.h>

#include <pdjson/pdjson.h>
#include <sn/sn.h>

#include <stdlib.h>

#include <templates/gitlab/snippets.h>

void
gcli_snippets_free(gcli_snippet *list, int const list_size)
{
	for (int i = 0; i < list_size; ++i) {
		free(list[i].title);
		free(list[i].filename);
		free(list[i].date);
		free(list[i].author);
		free(list[i].visibility);
		free(list[i].raw_url);
	}

	free(list);
}

int
gcli_snippets_get(int const max, gcli_snippet **const out)
{
	char               *url      = NULL;
	char               *next_url = NULL;
	gcli_fetch_buffer   buffer   = {0};
	struct json_stream  stream   = {0};
	size_t              size     = 0;

	*out = NULL;

	url = sn_asprintf("%s/snippets", gitlab_get_apibase());

	do {
		gcli_fetch(url, &next_url, &buffer);

		json_open_buffer(&stream, buffer.data, buffer.length);

		parse_gitlab_snippets(&stream, out, &size);

		json_close(&stream);
		free(url);
		free(buffer.data);
	} while ((url = next_url) && (max == -1 || (int)size < max));

	free(next_url);

	return (int)size;
}

static void
gcli_print_snippet(enum gcli_output_flags const flags,
                   gcli_snippet const *const it)
{
	printf("    ID : %d\n", it->id);
	printf(" TITLE : %s\n", it->title);
	printf("AUTHOR : %s\n", it->author);
	printf("  FILE : %s\n", it->filename);
	printf("  DATE : %s\n", it->date);
	printf("VSBLTY : %s\n", it->visibility);
	printf("   URL : %s\n\n", it->raw_url);
}

static void
gcli_print_snippets_long(enum gcli_output_flags const flags,
                         gcli_snippet const *const list,
                         int const list_size)
{
	if (flags & OUTPUT_SORTED) {
		for (int i = list_size; i > 0; --i)
			gcli_print_snippet(flags, &list[i - 1]);
	} else {
		for (int i = 0; i < list_size; ++i)
			gcli_print_snippet(flags, &list[i]);
	}
}

static void
gcli_print_snippets_short(enum gcli_output_flags const flags,
                          gcli_snippet const *const list,
                          int const list_size)
{
	gcli_tbl table;
	gcli_tblcoldef cols[] = {
		{ .name = "ID",         .type = GCLI_TBLCOLTYPE_INT,    .flags = GCLI_TBLCOL_JUSTIFYR },
		{ .name = "DATE",       .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "VISIBILITY", .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "AUTHOR",     .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "TITLE",      .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
	};

	if (gcli_tbl_init(cols, ARRAY_SIZE(cols), &table) < 0)
		errx(1, "error: could not init table");


	if (flags & OUTPUT_SORTED) {
		for (int i = list_size; i > 0; --i)
			gcli_tbl_add_row(table, list[i-1].id, list[i-1].date,
			                 list[i-1].visibility, list[i-1].author,
			                 list[i-1].title);
	} else {
		for (int i = 0; i < list_size; ++i)
			gcli_tbl_add_row(table, list[i].id, list[i].date,
			                 list[i].visibility, list[i].author, list[i].title);
	}

	gcli_tbl_dump(table);
	gcli_tbl_free(table);
}

void
gcli_snippets_print(enum gcli_output_flags const flags,
                    gcli_snippet const *const list,
                    int const list_size)
{
	if (list_size == 0) {
		puts("No Snippets");
		return;
	}

	if (flags & OUTPUT_LONG)
		gcli_print_snippets_long(flags, list, list_size);
	else
		gcli_print_snippets_short(flags, list, list_size);
}

void
gcli_snippet_delete(char const *snippet_id)
{
	gcli_fetch_buffer buffer = {0};
	char *url = sn_asprintf("%s/snippets/%s", gitlab_get_apibase(), snippet_id);

	gcli_fetch_with_method("DELETE", url, NULL, NULL, &buffer);

	free(url);
	free(buffer.data);
}

void
gcli_snippet_get(char const *snippet_id)
{
	char *url = sn_asprintf("%s/snippets/%s/raw",
	                        gitlab_get_apibase(),
	                        snippet_id);
	gcli_curl(stdout, url, NULL);
	free(url);
}
