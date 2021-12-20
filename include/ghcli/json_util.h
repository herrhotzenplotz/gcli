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

#ifndef JSON_UTIL_H
#define JSON_UTIL_H

#include <ghcli/curl.h>

#include <pdjson/pdjson.h>

#include <sn/sn.h>

#define get_int(input)     get_int_(input, __func__)
#define get_bool(input)    get_bool_(input, __func__)
#define get_string(input)  get_string_(input, __func__)
#define get_sv(input)      get_sv_(input, __func__)
#define get_user(input)    get_user_(input, __func__)
#define get_label(input)   get_label_(input, __func__)

int         get_int_(json_stream *input, const char *function);
bool        get_bool_(json_stream *input, const char *function);
char       *get_string_(json_stream *input, const char *function);
sn_sv       get_sv_(json_stream *input, const char *function);
char       *get_user_(json_stream *input, const char *function);
const char *get_label_(json_stream *input, const char *function);
sn_sv       ghcli_json_escape(sn_sv);
void        ghcli_print_html_url(ghcli_fetch_buffer);
size_t      ghcli_read_label_list(json_stream *, sn_sv **);
size_t      ghcli_read_user_list(json_stream *input, sn_sv **out);
void        ghcli_json_advance(json_stream *input, const char *fmt, ...);

static inline sn_sv
get_user_sv(json_stream *input)
{
    char *user_str = (char *)get_user(input);
    return SV(user_str);
}

static inline const char *
ghcli_json_bool(bool it)
{
    return it ? "true" : "false";
}

#endif /* JSON_UTIL_H */
