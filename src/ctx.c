/*
 * Copyright 2023-2025 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
		gcli_clear_ptr(&ctx->last_error);

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
gcli_set_progress_func(struct gcli_ctx *ctx,
                       void (*pfunc)(bool done))
{
	ctx->report_progress = pfunc;
}

char const *
gcli_get_apibase(struct gcli_ctx *ctx)
{
	if (!ctx->apibase)
		ctx->apibase = ctx->get_apibase(ctx);

	return ctx->apibase;
}

char const *
gcli_get_token(struct gcli_ctx *ctx)
{
	return ctx->get_token(ctx);
}

char *
gcli_get_authheader(struct gcli_ctx *ctx)
{
	char *hdr = NULL;
	char const *token = gcli_get_token(ctx);

	if (token && gcli_forge(ctx)->make_authheader) {
		hdr = gcli_forge(ctx)->make_authheader(ctx, token);
	}

	gcli_clear_ptr(&token);

	return hdr;
}

bool
gcli_be_verbose(struct gcli_ctx *ctx)
{
	return ctx->verbosity == GCLI_VERBOSITY_VERBOSE;
}

bool
gcli_be_quiet(struct gcli_ctx *ctx)
{
	return ctx->verbosity == GCLI_VERBOSITY_QUIET;
}

int
gcli_getverbosity(struct gcli_ctx *ctx)
{
	return ctx->verbosity;
}

void
gcli_setverbosity(struct gcli_ctx *ctx, int v)
{
	ctx->verbosity = v;
}

void
gcli_warn(struct gcli_ctx *ctx, char const *fmt, ...)
{
	if (!gcli_be_verbose(ctx))
		return;

	fputs("warning: ", stderr);
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fprintf(stderr, ": %s\n", strerror(errno));
}

void
gcli_warnx(struct gcli_ctx *ctx, char const *fmt, ...)
{
	if (!gcli_be_verbose(ctx))
		return;

	fputs("warning: ", stderr);
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fputc('\n', stderr);
}
