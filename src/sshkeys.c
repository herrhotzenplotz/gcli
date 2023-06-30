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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gcli/config.h>
#include <gcli/forges.h>
#include <gcli/sshkeys.h>
#include <gcli/table.h>

int
gcli_sshkeys_get_keys(gcli_sshkey_list *out)
{
	return gcli_forge()->get_sshkeys(out);
}

void
gcli_sshkeys_print_keys(gcli_sshkey_list const *list)
{
	gcli_tbl *tbl;
	gcli_tblcoldef cols[] = {
		{ .name = "ID",      .type = GCLI_TBLCOLTYPE_INT,    .flags = 0 },
		{ .name = "CREATED", .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "TITLE",   .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
	};

	if (list->keys_size == 0) {
		printf("No SSH keys\n");
		return;
	}

	tbl = gcli_tbl_begin(cols, ARRAY_SIZE(cols));

	for (size_t i = 0; i < list->keys_size; ++i) {
		gcli_tbl_add_row(tbl, list->keys[i].id, list->keys[i].created_at, list->keys[i].title);
	}

	gcli_tbl_end(tbl);
}

void
gcli_sshkeys_free_keys(gcli_sshkey_list *list)
{
	for (size_t i = 0; i < list->keys_size; ++i) {
		free(list->keys[i].title);
		free(list->keys[i].key);
		free(list->keys[i].created_at);
	}

	free(list->keys);

	list->keys = NULL;
	list->keys_size = 0;
}
