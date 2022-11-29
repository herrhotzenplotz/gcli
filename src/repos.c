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
gcli_get_repos(char const *owner, int const max, gcli_repo **const out)
{
	return gcli_forge()->get_repos(owner, max, out);
}


void
gcli_print_repos_table(enum gcli_output_flags const flags,
                       gcli_repo const *const repos,
                       size_t const repos_size)
{
	gcli_tbl table;
	gcli_tblcoldef cols[] = {
		{ .name = "FORK",     .type = GCLI_TBLCOLTYPE_BOOL, .flags = 0 },
		{ .name = "VISBLTY",  .type = GCLI_TBLCOLTYPE_SV,   .flags = 0 },
		{ .name = "DATE",     .type = GCLI_TBLCOLTYPE_SV,   .flags = 0 },
		{ .name = "FULLNAME", .type = GCLI_TBLCOLTYPE_SV,   .flags = 0 },
	};

	if (repos_size == 0) {
		puts("No repos");
		return;
	}


	/* init table */
	if (gcli_tbl_init(cols, ARRAY_SIZE(cols), &table) < 0)
		errx(1, "error: could not init table");

	/* put data into table */
	if (flags & OUTPUT_SORTED) {
		for (size_t i = repos_size; i > 0; --i)
			gcli_tbl_add_row(table, repos[i-1].is_fork, repos[i-1].visibility,
			                 repos[i-1].date, repos[i-1].full_name);
	} else {
		for (size_t i = 0; i < repos_size; ++i)
			gcli_tbl_add_row(table, repos[i].is_fork, repos[i].visibility,
			                 repos[i].date, repos[i].full_name);
	}

	/* print it */
	gcli_tbl_dump(table);
	gcli_tbl_free(table);
}

void
gcli_repos_free(gcli_repo *repos, size_t const repos_size)
{
	for (size_t i = 0; i < repos_size; ++i) {
		free(repos[i].full_name.data);
		free(repos[i].name.data);
		free(repos[i].owner.data);
		free(repos[i].date.data);
		free(repos[i].visibility.data);
	}

	free(repos);
}

int
gcli_get_own_repos(int const max, gcli_repo **const out)
{
	return gcli_forge()->get_own_repos(max, out);
}

void
gcli_repo_delete(char const *owner, char const *repo)
{
	gcli_forge()->repo_delete(owner, repo);
}

gcli_repo *
gcli_repo_create(
	gcli_repo_create_options const *options) /* Options descriptor */
{
	return gcli_forge()->repo_create(options);
}
