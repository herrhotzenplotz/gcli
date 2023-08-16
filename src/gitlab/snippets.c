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
#include <gcli/gitlab/snippets.h>
#include <gcli/json_util.h>
#include <gcli/cmd/table.h>

#include <pdjson/pdjson.h>
#include <sn/sn.h>

#include <stdlib.h>

#include <templates/gitlab/snippets.h>

void
gcli_snippets_free(gcli_snippet_list *const list)
{
	for (size_t i = 0; i < list->snippets_size; ++i) {
		free(list->snippets[i].title);
		free(list->snippets[i].filename);
		free(list->snippets[i].date);
		free(list->snippets[i].author);
		free(list->snippets[i].visibility);
		free(list->snippets[i].raw_url);
	}

	free(list->snippets);

	list->snippets = NULL;
	list->snippets_size = 0;
}

int
gcli_snippets_get(gcli_ctx *ctx, int const max, gcli_snippet_list *const out)
{
	char *url = NULL;

	gcli_fetch_list_ctx fl = {
		.listp = &out->snippets,
		.sizep = &out->snippets_size,
		.max = max,
		.parse = (parsefn)(parse_gitlab_snippets),
	};

	*out = (gcli_snippet_list) {0};
	url = sn_asprintf("%s/snippets", gitlab_get_apibase(ctx));

	return gcli_fetch_list(ctx, url, &fl);
}

static void
gcli_print_snippet(gcli_ctx *ctx, enum gcli_output_flags const flags,
                   gcli_snippet const *const it)
{
	gcli_dict dict;

	(void) flags;

	dict = gcli_dict_begin(ctx);

	gcli_dict_add(dict,        "ID",     0, 0, "%d", it->id);
	gcli_dict_add_string(dict, "TITLE",  0, 0, it->title);
	gcli_dict_add_string(dict, "AUTHOR", 0, 0, it->author);
	gcli_dict_add_string(dict, "FILE",   0, 0, it->filename);
	gcli_dict_add_string(dict, "DATE",   0, 0, it->date);
	gcli_dict_add_string(dict, "VSBLTY", 0, 0, it->visibility);
	gcli_dict_add_string(dict, "URL",    0, 0, it->raw_url);

	gcli_dict_end(dict);
}

static void
gcli_print_snippets_long(gcli_ctx *ctx, enum gcli_output_flags const flags,
                         gcli_snippet_list const *const list, int const max)
{
	int n;

	/* Determine number of items to print */
	if (max < 0 || (size_t)(max) > list->snippets_size)
		n = list->snippets_size;
	else
		n = max;

	if (flags & OUTPUT_SORTED) {
		for (int i = 0; i < n; ++i)
			gcli_print_snippet(ctx, flags, &list->snippets[n-i-1]);
	} else {
		for (int i = 0; i < n; ++i)
			gcli_print_snippet(ctx, flags, &list->snippets[i]);
	}
}

static void
gcli_print_snippets_short(gcli_ctx *ctx, enum gcli_output_flags const flags,
                          gcli_snippet_list const *const list, int const max)
{
	int n;
	gcli_tbl table;
	gcli_tblcoldef cols[] = {
		{ .name = "ID",         .type = GCLI_TBLCOLTYPE_INT,    .flags = GCLI_TBLCOL_JUSTIFYR },
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
	table = gcli_tbl_begin(ctx, cols, ARRAY_SIZE(cols));
	if (!table)
		errx(1, "error: could not init table");

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
gcli_snippets_print(gcli_ctx *ctx, enum gcli_output_flags const flags,
                    gcli_snippet_list const *const list, int const max)
{
	if (list->snippets_size == 0) {
		puts("No Snippets");
		return;
	}

	if (flags & OUTPUT_LONG)
		gcli_print_snippets_long(ctx, flags, list, max);
	else
		gcli_print_snippets_short(ctx, flags, list, max);
}

int
gcli_snippet_delete(gcli_ctx *ctx, char const *snippet_id)
{
	int rc = 0;
	char *url;

	url = sn_asprintf("%s/snippets/%s", gitlab_get_apibase(ctx), snippet_id);
	rc = gcli_fetch_with_method(ctx, "DELETE", url, NULL, NULL, NULL);

	free(url);

	return rc;
}

int
gcli_snippet_get(gcli_ctx *ctx, char const *snippet_id)
{
	int rc = 0;
	char *url = sn_asprintf("%s/snippets/%s/raw",
	                        gitlab_get_apibase(ctx),
	                        snippet_id);
	rc = gcli_curl(ctx, stdout, url, NULL);
	free(url);

	return rc;
}
