/*
 * Copyright Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/attachments.h>
#include <gcli/forges.h>

#include <stdlib.h>

void
gcli_attachments_free(struct gcli_attachment_list *list)
{
	for (size_t i = 0; i < list->attachments_size; ++i) {
		gcli_attachment_free(&list->attachments[i]);
	}

	gcli_clear_ptr(&list->attachments);
	list->attachments_size = 0;
}

void
gcli_attachment_free(struct gcli_attachment *it)
{
	gcli_clear_ptr(&it->author);
	gcli_clear_ptr(&it->file_name);
	gcli_clear_ptr(&it->summary);
	gcli_clear_ptr(&it->content_type);
	gcli_clear_ptr(&it->data_base64);
}

int
gcli_attachment_get_content(struct gcli_ctx *const ctx, gcli_id const id, FILE *out)
{
	gcli_null_check_call(attachment_get_content, ctx, id, out);
}

int
gcli_attachment_create(struct gcli_ctx *const ctx,
                       struct gcli_attachment_create_opts const *const flags)
{
	gcli_null_check_call(create_attachment, ctx, flags);
}
