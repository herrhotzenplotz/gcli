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

#ifndef GCLI_PORT_UTIL_H
#define GCLI_PORT_UTIL_H

#include <gcli/port/err.h>

#include <stdbool.h>
#include <stdio.h>

/* for convenience */
#define gcli_unimplemented errx(42, "%s: unimplemented", __func__)
#define gcli_notreached    errx(42, "%s: unreachable", __func__)

/* Weird minimum function */
static inline int
gcli_min(int x, int y)
{
	if (x < 0)
		return y;
	else if (y < 0)
		return x;
	else if (x < y)
		return x;
	else
		return y;
}

int gcli_read_file(char const *path, char **buffer);
int gcli_read_stream(FILE *stream, char **buffer);

/* interactive user functions */
bool gcli_yesno(const char *fmt, ...) PRINTF_FORMAT(1, 2);

static inline const char *
gcli_bool_yesno(bool x)
{
	return x ? "yes" : "no";
}


#endif /* GCLI_PORT_UTIL_H */
