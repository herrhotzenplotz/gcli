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

#include <gcli/gcli.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

char const *
gcli_init(struct gcli_ctx **ctx,
          gcli_forge_type (*get_forge_type)(struct gcli_ctx *),
          char const *(*get_token)(struct gcli_ctx *),
          char const *(*get_apibase)(struct gcli_ctx *))
{
	*ctx = calloc(1, sizeof (struct gcli_ctx));
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
		struct gcli_ctx *c = *ctx;

		gcli_clear_ptr(&c->apibase);
		gcli_clear_ptr(&c->last_error);

		c = NULL;
		gcli_clear_ptr(ctx);

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

void
gcli_clear_ptr(void *ptr)
{
	void **_ptr = ptr;

	free(*_ptr);
	*_ptr = NULL;
}

int
gcli_parse_forgetype(struct gcli_ctx *ctx, char const *const in,
                     gcli_forge_type *const out)
{
	int rc = 0;

	if (strcmp(in, "github") == 0)
		*out = GCLI_FORGE_GITHUB;
	else if (strcmp(in, "gitlab") == 0)
		*out = GCLI_FORGE_GITLAB;
	else if (strcmp(in, "gitea") == 0)
		*out = GCLI_FORGE_GITEA;
	else if (strcmp(in, "bugzilla") == 0)
		*out = GCLI_FORGE_BUGZILLA;
	else
		rc = gcli_error(ctx, "bad forge type %s", in);

	return rc;
}
