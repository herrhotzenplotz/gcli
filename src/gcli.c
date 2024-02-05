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

#include <gcli/gcli.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

char const *
gcli_init(struct gcli_ctx **ctx,
          gcli_forge_type (*get_forge_type)(struct gcli_ctx *),
          char *(*get_token)(struct gcli_ctx *),
          char *(*get_apibase)(struct gcli_ctx *))
{
	*ctx = calloc(sizeof (struct gcli_ctx), 1);
	if (!(*ctx))
		return strerror(errno);

	(*ctx)->get_forge_type = get_forge_type;
	(*ctx)->get_token = get_token;
	(*ctx)->get_apibase = get_apibase;

	(*ctx)->apibase = NULL;

	return NULL;
}

void
gcli_destroy(struct gcli_ctx **ctx)
{
	if (ctx && *ctx) {
		free((*ctx)->apibase);

		free(*ctx);
		*ctx = NULL;

		/* TODO: other deinit stuff? */
	}
}

char const *
gcli_get_error(struct gcli_ctx *ctx)
{
	if (ctx->last_error)
		return ctx->last_error;
	else
		return "No error";
}
