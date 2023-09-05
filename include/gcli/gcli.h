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

#ifndef GCLI_H
#define GCLI_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

enum gcli_output_flags {
	OUTPUT_SORTED = (1 << 0),
	OUTPUT_LONG   = (1 << 1),
};

typedef enum gcli_forge_type {
	GCLI_FORGE_GITHUB,
	GCLI_FORGE_GITLAB,
	GCLI_FORGE_GITEA,
} gcli_forge_type;

typedef unsigned int gcli_id;

#ifdef IN_LIBGCLI
#include <gcli/ctx.h>
#endif /* IN_LIBGCLI */

typedef struct gcli_ctx gcli_ctx;

char const *gcli_init(gcli_ctx **,
                      gcli_forge_type (*get_forge_type)(gcli_ctx *),
                      char *(*get_authheader)(gcli_ctx *),
                      char *(*get_apibase)(gcli_ctx *));

void *gcli_get_userdata(struct gcli_ctx const *);
void gcli_set_userdata(struct gcli_ctx *, void *usrdata);
void gcli_destroy(gcli_ctx **ctx);
char const *gcli_get_error(gcli_ctx *ctx);

#endif /* GCLI_H */
