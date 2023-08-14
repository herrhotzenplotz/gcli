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

#ifndef JSON_UTIL_H
#define JSON_UTIL_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gcli/curl.h>

#include <pdjson/pdjson.h>

#include <sn/sn.h>

#include <stdint.h>

#define get_int(ctx, input)       get_int_(ctx, input, __func__)
#define get_double(ctx, input)    get_double_(ctx, input, __func__)
#define get_parse_int(ctx, input) get_parse_int_(ctx, input, __func__)
#define get_bool(ctx, input)      get_bool_(ctx, input, __func__)
#define get_string(ctx, input)    get_string_(ctx, input, __func__)
#define get_sv(ctx, input)        get_sv_(ctx, input, __func__)
#define get_user(ctx, input)      get_user_(ctx, input, __func__)
#define get_label(ctx, input)     get_label_(ctx, input, __func__)
#define get_is_string(ctx, input) ((void)ctx, json_next(input) == JSON_STRING)
#define get_int_to_sv(ctx, input) (sn_sv_fmt("%ld", get_int(ctx, input)))
#define get_int_to_string(ctx, input) (sn_asprintf("%ld", get_int(ctx, input)))

long        get_int_(gcli_ctx *ctx, json_stream *input, char const *function);
double      get_double_(gcli_ctx *ctx, json_stream *input, char const *function);
long        get_parse_int_(gcli_ctx *ctx, json_stream *input,
                           char const *function);
bool        get_bool_(gcli_ctx *ctx, json_stream *input, char const *function);
char       *get_string_(gcli_ctx *ctx, json_stream *input, char const *function);
sn_sv       get_sv_(gcli_ctx *ctx, json_stream *input, char const *function);
char       *get_user_(gcli_ctx *ctx, json_stream *input, char const *function);
char const *get_label_(json_stream *input, char const *function);
sn_sv       gcli_json_escape(sn_sv);
#define     gcli_json_escape_cstr(x) (gcli_json_escape(SV((char *)(x))).data)
void        gcli_print_html_url(gcli_ctx *, gcli_fetch_buffer);
void        gcli_json_advance(json_stream *input, char const *fmt, ...);
uint32_t    get_github_style_colour(gcli_ctx *ctx, json_stream *input);
uint32_t    get_gitlab_style_colour(gcli_ctx *ctx, json_stream *input);
int         get_github_is_pr(gcli_ctx *ctx, json_stream *input);
bool        get_gitlab_can_be_merged(gcli_ctx *ctx, json_stream *input);
sn_sv       get_gitea_visibility(gcli_ctx *ctx, json_stream *input);

static inline sn_sv
get_user_sv(gcli_ctx *ctx, json_stream *input)
{
	char *user_str = (char *)get_user(ctx, input);
	return SV(user_str);
}

static inline void
parse_user(gcli_ctx *ctx, json_stream *input, sn_sv *out)
{
	*out = get_user_sv(ctx, input);
}

static inline char const *
gcli_json_bool(bool it)
{
	return it ? "true" : "false";
}

#define SKIP_OBJECT_VALUE(stream)	  \
	do { \
		enum json_type value_type = json_next(stream); \
	  \
		switch (value_type) { \
		case JSON_ARRAY: \
			json_skip_until(stream, JSON_ARRAY_END); \
			break; \
		case JSON_OBJECT: \
			json_skip_until(stream, JSON_OBJECT_END); \
			break; \
		default: \
			break; \
		} \
	} while (0)

static inline void
parse_sv(gcli_ctx *ctx, json_stream *stream, sn_sv *out)
{
	(void) *ctx;
	*out = get_sv(ctx, stream);
}

#endif /* JSON_UTIL_H */
