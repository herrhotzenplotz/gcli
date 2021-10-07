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

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sn/sn.h>

void
errx(int code, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fputc('\n', stderr);
    exit(code);
}

void
err(int code, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fprintf(stderr, ": %s\n", strerror(errno));
    exit(code);
}

char *
sn_strndup(const char *it, size_t len)
{
    size_t actual = 0;
    const char *tmp = NULL;
    char *result = NULL;

    tmp = it;

    while (tmp[actual++] && actual < len);

    result = calloc(1, actual + 1);
    memcpy(result, it, actual);
    return result;
}

char *
sn_asprintf(const char *fmt, ...)
{
    char    tmp    = 0, *result = NULL;
    size_t  actual = 0;
    va_list vp;

    va_start(vp, fmt);

    actual = vsnprintf(&tmp, 1, fmt, vp);
    va_end(vp);
    result = calloc(1, actual + 1);

    va_start(vp, fmt);
    vsnprintf(result, actual + 1, fmt, vp);
    va_end(vp);

    return result;
}

static int
word_length(const char *x)
{
    int l = 0;

    while (*x && !isspace(*x++))
        l++;
    return l;
}

void
pretty_print(const char *input, int indent, int maxlinelen, FILE *out)
{
    const char *it = input;
    while (*it) {
        int linelength = indent;
        fprintf(out, "%*.*s", indent, indent, "");

        do {
            int w = word_length(it) + 1;

            if (it[w - 1] == '\n') {
                fprintf(out, "%.*s", w - 1, it);
                it += w;
                break;
            }

            fprintf(out, "%.*s", w, it);
            it += w;
            linelength += w;


        } while (*it && (linelength < maxlinelen));
        fputc('\n', out);
    }
}

int
sn_mmap_file(const char *path, void **buffer)
{
    struct stat stat_buf = {0};
    int         fd       = 0;

    if (access(path, R_OK) < 0)
        err(1, "access");

    if (stat(path, &stat_buf) < 0)
        err(1, "stat");

    if ((fd = open(path, O_RDONLY)) < 0)
        err(1, "open");

    *buffer = mmap(NULL, stat_buf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (*buffer == MAP_FAILED)
        err(1, "mmap");

    return stat_buf.st_size;
}

sn_sv
sn_sv_trim_front(sn_sv it)
{
    if (it.length == 0)
        return it;

    // TODO: not utf-8 aware
    while (it.length > 0) {
        if (!isspace(*it.data))
            break;

        it.data++;
        it.length--;
    }

    return it;
}

sn_sv
sn_sv_chop_until(sn_sv *it, char c)
{
    sn_sv result = *it;

    result.length = 0;

    while (it->length > 0) {

        if (*it->data == c)
            break;

        it->data++;
        it->length--;
        result.length++;
    }

    return result;
}

bool
sn_sv_has_prefix(sn_sv it, const char *prefix)
{
    size_t len = strlen(prefix);

    if (it.length < len)
        return false;

    return strncmp(it.data, prefix, len) == 0;
}
