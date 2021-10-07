/*
 * Copyright 2021 Nico Sonack <nsonack@outlook.com>
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

/*
 * LibSN - things I reuse all the time.
 */

#ifndef SN_H
#define SN_H

#include <stdio.h>
#include <stdbool.h>

/* error functions */
void errx(int code, const char *fmt, ...);
void err(int code, const char *fmt, ...);

/* string functions */
char *sn_strndup (const char *it, size_t len);
char *sn_asprintf(const char *fmt, ...);

/* pretty functions */
void pretty_print(const char *input, int indent, int maxlinelen, FILE *out);

/* io file mapping */
int  sn_mmap_file(const char *path, void **buffer);

/* stringview */
typedef struct sn_sv sn_sv;

struct sn_sv {
    char   *data;
    size_t  length;
};

sn_sv sn_sv_trim_front(sn_sv);
sn_sv sn_sv_chop_until(sn_sv *, char);
bool  sn_sv_has_prefix(sn_sv, const char *);

#endif /* SN_H */
