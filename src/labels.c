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

#include <gcli/color.h>
#include <gcli/forges.h>
#include <gcli/labels.h>
#include <gcli/table.h>

size_t
gcli_get_labels(char const *owner,
                char const *reponame,
                int const max,
                gcli_label **const out)
{
	return gcli_forge()->get_labels(owner, reponame, max, out);
}

void
gcli_free_label(gcli_label *const label)
{
	free(label->name);
	free(label->description);
}

void
gcli_free_labels(gcli_label *labels, size_t const labels_size)
{
	for (size_t i = 0; i < labels_size; ++i)
		gcli_free_label(&labels[i]);
	free(labels);
}

void
gcli_print_labels(gcli_label const *const labels, size_t const labels_size)
{
	gcli_tbl table;
	gcli_tblcoldef cols[] = {
		{ .name = "ID",          .type = GCLI_TBLCOLTYPE_INT,    .flags = GCLI_TBLCOL_JUSTIFYR },
		{ .name = "NAME",        .type = GCLI_TBLCOLTYPE_STRING, .flags = GCLI_TBLCOL_256COLOUR|GCLI_TBLCOL_BOLD },
		{ .name = "DESCRIPTION", .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
	};

	table = gcli_tbl_begin(cols, ARRAY_SIZE(cols));
	if (!table)
		errx(1, "error: could not init table");

	for (size_t i = 0; i < labels_size; ++i) {
		gcli_tbl_add_row(table, labels[i].id, labels[i].color, labels[i].name,
		                 labels[i].description);
	}

	gcli_tbl_end(table);
}

void
gcli_create_label(char const *owner, char const *repo, gcli_label *const label)
{
	gcli_forge()->create_label(owner, repo, label);
}

void
gcli_delete_label(char const *owner, char const *repo, char const *const label)
{
	gcli_forge()->delete_label(owner, repo, label);
}
