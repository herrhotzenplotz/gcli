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

#include <gcli/forges.h>
#include <gcli/github/repos.h>
#include <gcli/repos.h>
#include <gcli/table.h>

#include <stdlib.h>

int
gcli_get_repos(gcli_ctx *ctx, char const *owner, int const max,
               gcli_repo_list *const out)
{
	return gcli_forge(ctx)->get_repos(ctx, owner, max, out);
}


void
gcli_print_repos_table(gcli_ctx *ctx, enum gcli_output_flags const flags,
                       gcli_repo_list const *const list, int const max)
{
	size_t n;
	gcli_tbl table;
	gcli_tblcoldef cols[] = {
		{ .name = "FORK",     .type = GCLI_TBLCOLTYPE_BOOL, .flags = 0 },
		{ .name = "VISBLTY",  .type = GCLI_TBLCOLTYPE_SV,   .flags = 0 },
		{ .name = "DATE",     .type = GCLI_TBLCOLTYPE_SV,   .flags = 0 },
		{ .name = "FULLNAME", .type = GCLI_TBLCOLTYPE_SV,   .flags = 0 },
	};

	if (list->repos_size == 0) {
		puts("No repos");
		return;
	}

	/* Determine number of repos to print */
	if (max < 0 || (size_t)(max) > list->repos_size)
		n = list->repos_size;
	else
		n = max;

	/* init table */
	table = gcli_tbl_begin(ctx, cols, ARRAY_SIZE(cols));
	if (!table)
		errx(1, "error: could not init table");

	/* put data into table */
	if (flags & OUTPUT_SORTED) {
		for (size_t i = 0; i < n; ++i)
			gcli_tbl_add_row(table,
			                 list->repos[n-i-1].is_fork,
			                 list->repos[n-i-1].visibility,
			                 list->repos[n-i-1].date,
			                 list->repos[n-i-1].full_name);
	} else {
		for (size_t i = 0; i < n; ++i)
			gcli_tbl_add_row(table,
			                 list->repos[i].is_fork,
			                 list->repos[i].visibility,
			                 list->repos[i].date,
			                 list->repos[i].full_name);
	}

	/* print it */
	gcli_tbl_end(table);
}

void
gcli_repo_print(gcli_ctx *ctx, gcli_repo const *it)
{
	gcli_dict dict;

	dict = gcli_dict_begin(ctx);
	gcli_dict_add(dict, "ID",         0, 0, "%d", it->id);
	gcli_dict_add(dict, "FULL NAME",  0, 0, SV_FMT, SV_ARGS(it->full_name));
	gcli_dict_add(dict, "NAME",       0, 0, SV_FMT, SV_ARGS(it->name));
	gcli_dict_add(dict, "OWNER",      0, 0, SV_FMT, SV_ARGS(it->owner));
	gcli_dict_add(dict, "DATE",       0, 0, SV_FMT, SV_ARGS(it->date));
	gcli_dict_add(dict, "VISIBILITY", 0, 0, SV_FMT, SV_ARGS(it->visibility));
	gcli_dict_add(dict, "IS FORK",    0, 0, "%s", sn_bool_yesno(it->is_fork));

	gcli_dict_end(dict);
}

void
gcli_repo_free(gcli_repo *it)
{
	free(it->full_name.data);
	free(it->name.data);
	free(it->owner.data);
	free(it->date.data);
	free(it->visibility.data);
	memset(it, 0, sizeof(*it));
}

void
gcli_repos_free(gcli_repo_list *const list)
{
	for (size_t i = 0; i < list->repos_size; ++i) {
		gcli_repo_free(&list->repos[i]);
	}

	free(list->repos);

	list->repos = NULL;
	list->repos_size = 0;
}

int
gcli_get_own_repos(gcli_ctx *ctx, int const max, gcli_repo_list *const out)
{
	return gcli_forge(ctx)->get_own_repos(ctx, max, out);
}

int
gcli_repo_delete(gcli_ctx *ctx, char const *owner, char const *repo)
{
	return gcli_forge(ctx)->repo_delete(ctx, owner, repo);
}

int
gcli_repo_create(gcli_ctx *ctx, gcli_repo_create_options const *options,
                 gcli_repo *out)
{
	return gcli_forge(ctx)->repo_create(ctx, options, out);
}
