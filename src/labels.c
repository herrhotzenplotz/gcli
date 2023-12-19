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

#include <gcli/forges.h>
#include <gcli/labels.h>

int
gcli_get_labels(gcli_ctx *ctx, char const *owner, char const *reponame,
                int const max, gcli_label_list *const out)
{
	gcli_null_check_call(get_labels, ctx, owner, reponame, max, out);
}

void
gcli_free_label(gcli_label *const label)
{
	free(label->name);
	free(label->description);
}

void
gcli_free_labels(gcli_label_list *const list)
{
	for (size_t i = 0; i < list->labels_size; ++i)
		gcli_free_label(&list->labels[i]);
	free(list->labels);

	list->labels = NULL;
	list->labels_size = 0;
}

int
gcli_create_label(gcli_ctx *ctx, char const *owner, char const *repo,
                  gcli_label *const label)
{
	gcli_null_check_call(create_label, ctx, owner, repo, label);
}

int
gcli_delete_label(gcli_ctx *ctx, char const *owner, char const *repo,
                  char const *const label)
{
	gcli_null_check_call(delete_label, ctx, owner, repo, label);
}
