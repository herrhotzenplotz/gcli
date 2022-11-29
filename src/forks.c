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

#include <gcli/color.h>
#include <gcli/forges.h>
#include <gcli/forks.h>
#include <gcli/github/forks.h>
#include <gcli/table.h>

int
gcli_get_forks(char const *owner,
               char const *repo,
               int const max,
               gcli_fork **const out)
{
	return gcli_forge()->get_forks(owner, repo, max, out);
}

void
gcli_print_forks(enum gcli_output_flags const flags,
                 gcli_fork const *const forks,
                 size_t const forks_size)
{
	gcli_tbl table;
	gcli_tblcoldef cols[] = {
		{ .name = "OWNER",    .type = GCLI_TBLCOLTYPE_SV,  .flags = GCLI_TBLCOL_BOLD },
		{ .name = "DATE",     .type = GCLI_TBLCOLTYPE_SV,  .flags = 0 },
		{ .name = "FORKS",    .type = GCLI_TBLCOLTYPE_INT, .flags = GCLI_TBLCOL_JUSTIFYR },
		{ .name = "FULLNAME", .type = GCLI_TBLCOLTYPE_SV,  .flags = 0 },
	};

	if (forks_size == 0) {
		puts("No forks");
		return;
	}


	table = gcli_tbl_begin(cols, ARRAY_SIZE(cols));
	if (!table)
		errx(1, "error: could not initialize table");

	if (flags & OUTPUT_SORTED) {
		for (size_t i = forks_size; i > 0; --i) {
			gcli_tbl_add_row(table, forks[i-1].owner, forks[i-1].date,
			                 forks[i-1].forks, forks[i-1].full_name);
		}
	} else {
		for (size_t i = 0; i < forks_size; ++i) {
			gcli_tbl_add_row(table, forks[i].owner, forks[i].date,
			                 forks[i].forks, forks[i].full_name);
		}
	}

	gcli_tbl_end(table);
}

void
gcli_fork_create(char const *owner, char const *repo, char const *_in)
{
	gcli_forge()->fork_create(owner, repo, _in);
}
