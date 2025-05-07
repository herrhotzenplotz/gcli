/*
 * Copyright 2021-2025 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#ifndef GCLI_PORT_SV_H
#define GCLI_PORT_SV_H

#include <stdbool.h>
#include <string.h>

#include <gcli/port/port.h>

/* stringview */
typedef struct sv gcli_sv;

struct sv {
	char *data;
	size_t length;
};

#define SV(x) (gcli_sv) { .data = x, .length = strlen(x) }
#define SV_FMT "%.*s"
#define SV_ARGS(x) (int)x.length, x.data
#define SV_NULL (gcli_sv) {0}

static inline gcli_sv
gcli_sv_from_parts(char *buf, size_t len)
{
	return (gcli_sv) { .data = buf, .length = len };
}

gcli_sv gcli_sv_trim_front(gcli_sv);
gcli_sv gcli_sv_trim(gcli_sv);
gcli_sv gcli_sv_chop_until(gcli_sv *, char);
gcli_sv gcli_sv_chop_to_last(gcli_sv *, char);
bool gcli_sv_has_prefix(gcli_sv, const char *);
bool gcli_sv_eq(const gcli_sv, const gcli_sv);
bool gcli_sv_eq_to(const gcli_sv, const char *);
gcli_sv gcli_sv_fmt(const char *fmt, ...) PRINTF_FORMAT(1, 2);
char *gcli_sv_to_cstr(gcli_sv);
gcli_sv gcli_sv_strip_suffix(gcli_sv, const char *suffix);
gcli_sv gcli_sv_append(gcli_sv this, gcli_sv const that);

static inline bool
gcli_sv_null(gcli_sv it)
{
	return it.data == NULL && it.length == 0;
}

#endif // GCLI_PORT_SV_H
