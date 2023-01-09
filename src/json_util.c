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

#include <gcli/json_util.h>
#include <gcli/forges.h>
#include <sn/sn.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

static inline void
barf(char const *message, char const *where)
{
	errx(1,
	     "error: %s.\n"
	     "       This might be an error on the GitHub api or the result of\n"
	     "       incorrect usage of cli flags. See gcli(1) to make sure your\n"
	     "       flags are correct. If you are certain that all your options\n"
	     "       were correct, please submit a bug report including the\n"
	     "       command you invoked and the following information about the\n"
	     "       error location: function = %s", message, where);
}

long
get_int_(json_stream *const input, char const *where)
{
	if (json_next(input) != JSON_NUMBER)
		barf("unexpected non-integer field", where);

	return json_get_number(input);
}

double
get_double_(json_stream *const input, char const *where)
{
	enum json_type type = json_next(input);

	/* This is dumb but it fixes a couple of weirdnesses of the API */
	if (type == JSON_NULL)
		return 0;
	if (type == JSON_NUMBER)
		return json_get_number(input);

	barf("unexpected non-double field", where);
	return 0; /* NOTREACHED */
}

char *
get_string_(json_stream *const input, char const *where)
{
	enum json_type const type = json_next(input);
	if (type == JSON_NULL)
		return strdup("<empty>");

	if (type != JSON_STRING)
		barf("unexpected non-string field", where);

	size_t len;
	char const *it = json_get_string(input, &len);

	if (!it)
		return strdup("<empty>");
	else
		return sn_strndup(it, len);
}

bool
get_bool_(json_stream *const input, char const *where)
{
	enum json_type value_type = json_next(input);
	if (value_type == JSON_TRUE)
		return true;
	else if (value_type == JSON_FALSE || value_type == JSON_NULL) // HACK
		return false;
	else
		barf("unexpected non-boolean value", where);

	errx(42, "%s: unreachable", where);
	return false;
}

char *
get_user_(json_stream *const input, char const *where)
{
	if (json_next(input) != JSON_OBJECT)
		barf("user field is not an object", where);

	char const *expected_key = gcli_forge()->user_object_key;

	char *result = NULL;
	while (json_next(input) == JSON_STRING) {
		size_t      len = 0;
		char const *key = json_get_string(input, &len);

		if (strncmp(expected_key, key, len) == 0) {
			if (json_next(input) != JSON_STRING)
				barf(
					"login of the pull request creator is not a string",
					where);

			char const *tmp = json_get_string(input, &len);
			result = sn_strndup(tmp, len);
		} else {
			json_next(input);
		}
	}

	return result;
}

static struct {
	char        c;
	char const *with;
} json_escape_table[] = {
	{ .c = '\n', .with = "\\n"  },
	{ .c = '\t', .with = "\\t"  },
	{ .c = '\r', .with = "\\r"  },
	{ .c = '\\', .with = "\\\\" },
	{ .c = '"' , .with = "\\\"" },
};

sn_sv
gcli_json_escape(sn_sv const it)
{
	sn_sv result = {0};

	result.data = malloc(2 * it.length);
	if (!result.data)
		err(1, "malloc");

	for (size_t i = 0; i < it.length; ++i) {
		for (size_t c = 0; c < ARRAY_SIZE(json_escape_table); ++c) {
			if (json_escape_table[c].c == it.data[i]) {
				size_t const len = strlen(json_escape_table[c].with);
				memcpy(result.data + result.length,
				       json_escape_table[c].with,
				       len);
				result.length += len;
				goto next;
			}
		}

		memcpy(result.data + result.length, it.data + i, 1);
		result.length += 1;
	next:
		continue;
	}

	return result;
}

sn_sv
get_sv_(json_stream *const input, char const *where)
{
	enum json_type type = json_next(input);
	if (type == JSON_NULL)
		return SV_NULL;

	if (type != JSON_STRING)
		barf("unexpected non-string field", where);

	size_t len;
	char const *it   = json_get_string(input, &len);
	char       *copy = sn_strndup(it, len);
	return SV(copy);
}

void
gcli_print_html_url(gcli_fetch_buffer const buffer)
{
	json_stream stream = {0};

	json_open_buffer(&stream, buffer.data, buffer.length);
	json_set_streaming(&stream, true);

	enum json_type next = json_next(&stream);
	char const *expected_key = gcli_forge()->html_url_key;

	while ((next = json_next(&stream)) == JSON_STRING) {
		size_t len;

		char const *key = json_get_string(&stream, &len);
		if (strncmp(key, expected_key, len) == 0) {
			char *url = get_string(&stream);
			puts(url);
			free(url);
		} else {
			enum json_type value_type = json_next(&stream);

			switch (value_type) {
			case JSON_ARRAY:
				json_skip_until(&stream, JSON_ARRAY_END);
				break;
			case JSON_OBJECT:
				json_skip_until(&stream, JSON_OBJECT_END);
				break;
			default:
				break;
			}
		}
	}

	if (next != JSON_OBJECT_END)
		barf("unexpected key type in json object", __func__);

	json_close(&stream);
}


char const *
get_label_(json_stream *const input, char const *where)
{
	if (json_next(input) != JSON_OBJECT)
		barf("label field is not an object", where);

	char const *result = NULL;
	while (json_next(input) == JSON_STRING) {
		size_t      len = 0;
		char const *key = json_get_string(input, &len);

		if (strncmp("name", key, len) == 0) {
			if (json_next(input) != JSON_STRING)
				barf("name of the label is not a string", where);

			result = json_get_string(input, &len);
			result = sn_strndup(result, len);
		} else {
			json_next(input);
		}
	}

	return result;
}


void
gcli_json_advance(json_stream *const stream, char const *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	while (*fmt) {
		switch (*fmt++) {
		case '[': {
			if (json_next(stream) != JSON_ARRAY)
				errx(1, "Expected array begin");
		} break;
		case '{': {
			if (json_next(stream) != JSON_OBJECT)
				errx(1, "Expected array begin");
		} break;
		case 's': {
			if (json_next(stream) != JSON_STRING)
				errx(1, "Expected string");

			char       *it    = va_arg(ap, char *);
			size_t      len   = 0;
			char const *other = json_get_string(stream, &len);
			if (strncmp(it, other, len))
				errx(1, "String unmatched");
		} break;
		case ']': {
			if (json_next(stream) != JSON_ARRAY_END)
				errx(1, "Expected array end");
		}   break;
		case '}': {
			if (json_next(stream) != JSON_OBJECT_END)
				errx(1, "Expected object end");
		}   break;
		case 'i': {
			if (json_next(stream) != JSON_NUMBER)
				errx(1, "Expected integer");
		}   break;
		}
	}

	va_end(ap);
}

long
get_parse_int_(json_stream *const input, char const *function)
{
	long  result = 0;
	char *endptr = NULL;
	char *string = get_string(input);

	result = strtol(string, &endptr, 10);
	if (endptr != string + strlen(string))
		err(1, "in %s: unable to parse string field into decimal integer",
		    function);

	return result;
}

uint32_t
get_github_style_colour(json_stream *const input)
{
	char *colour_str = get_string(input);
	char *endptr    = NULL;

	unsigned long colour = strtoul(colour_str, &endptr, 16);
	if (endptr != colour_str + strlen(colour_str))
		errx(1, "error: the api returned an"
		     "invalid hexadecimal colour code");

	free(colour_str);

	return ((uint32_t)(colour)) << 8;
}

uint32_t
get_gitlab_style_colour(json_stream *const input)
{
	char *colour  = get_string(input);
	char *endptr = NULL;
	long  code   = 0;

	code = strtol(colour + 1, &endptr, 16);
	if (endptr != (colour + 1 + strlen(colour + 1)))
		err(1, "error: colour code is invalid");

	free(colour);

	return ((uint32_t)(code) << 8);
}

sn_sv
get_gitea_visibility(json_stream *const input)
{
	char *v = NULL;
	if (get_bool(input))
		v = strdup("private");
	else
		v = strdup("public");
	return SV(v);
}

bool
get_gitlab_can_be_merged(json_stream *const input)
{
	return sn_sv_eq_to(get_sv(input), "can_be_merged");
}

int
get_github_is_pr(json_stream *input)
{
	enum json_type next = json_peek(input);

	if (next == JSON_NULL)
		json_next(input);
	else
		SKIP_OBJECT_VALUE(input);

	return next == JSON_OBJECT;
}
