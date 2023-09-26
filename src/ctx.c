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

#include <gcli/forges.h>
#include <gcli/gcli.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

int
gcli_error(struct gcli_ctx *ctx, char const *const fmt, ...)
{
	va_list vp;
	char *buf;
	size_t len;

	va_start(vp, fmt);
	len = vsnprintf(NULL, 0, fmt, vp);
	va_end(vp);

	buf = malloc(len + 1);

	va_start(vp, fmt);
	vsnprintf(buf, len + 1, fmt, vp);
	va_end(vp);

	if (ctx->last_error)
		free(ctx->last_error);

	ctx->last_error = buf;

	return -1;
}

void *
gcli_get_userdata(struct gcli_ctx const *ctx)
{
	return ctx->usrdata;
}

void
gcli_set_userdata(struct gcli_ctx *ctx, void *usrdata)
{
	ctx->usrdata = usrdata;
}

void
gcli_set_progress_func(struct gcli_ctx *ctx, void (*pfunc)(void))
{
	ctx->report_progress = pfunc;
}

char *
gcli_get_apibase(struct gcli_ctx *ctx)
{
	return ctx->get_apibase(ctx);
}

char *
gcli_get_authheader(struct gcli_ctx *ctx)
{
	char *hdr = NULL;
	char *token = ctx->get_token(ctx);

	if (token) {
		hdr = gcli_forge(ctx)->make_authheader(ctx, token);
		free(token);
	}

	return hdr;
}
