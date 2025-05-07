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

#include <gcli/port/string.h>
#include <gcli/port/sv.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

gcli_sv
gcli_sv_append(gcli_sv this, gcli_sv const that)
{
	/* Allocate one byte more as we're going to manually zero-terminate the result
	 * down in get_message */
	this.data = realloc(this.data, this.length + that.length + 1);
	memcpy(this.data + this.length, that.data, that.length);
	this.length += that.length;

	return this;
}

char *
gcli_sv_to_cstr(gcli_sv it)
{
	return gcli_strndup(it.data, it.length);
}

bool
gcli_sv_eq_to(const gcli_sv this, const char *that)
{
	size_t len = strlen(that);
	if (len != this.length)
		return false;

	return strncmp(this.data, that, len) == 0;
}

gcli_sv
gcli_sv_fmt(const char *fmt, ...)
{
	char tmp = 0;
	va_list vp;
	gcli_sv result = {0};

	va_start(vp, fmt);

	result.length = vsnprintf(&tmp, 1, fmt, vp);
	va_end(vp);
	result.data = calloc(1, result.length + 1);

	va_start(vp, fmt);
	vsnprintf(result.data, result.length + 1, fmt, vp);
	va_end(vp);

	return result;
}

gcli_sv
gcli_sv_trim_front(gcli_sv it)
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

bool
gcli_sv_has_prefix(gcli_sv it, const char *prefix)
{
	size_t len = strlen(prefix);

	if (it.length < len)
		return false;

	return strncmp(it.data, prefix, len) == 0;
}

gcli_sv
gcli_sv_chop_until(gcli_sv *it, char c)
{
	gcli_sv result = *it;

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

static gcli_sv
gcli_sv_trim_end(gcli_sv it)
{
	while (it.length > 0 && isspace(it.data[it.length - 1]))
		it.length--;

	return it;
}

gcli_sv
gcli_sv_trim(gcli_sv it)
{
	return gcli_sv_trim_front(gcli_sv_trim_end(it));
}

gcli_sv
gcli_sv_chop_to_last(gcli_sv *it, char sep)
{
	gcli_sv result = *it;

	while (result.length) {
		if (result.data[--result.length] == sep)
			break;
	}

	it->length -= result.length;
	it->data += result.length;

	return result;
}

gcli_sv
gcli_sv_strip_suffix(gcli_sv input, const char *suffix)
{
	gcli_sv expected_suffix = SV((char *)suffix);

	if (input.length < expected_suffix.length)
		return input;

	gcli_sv actual_suffix = gcli_sv_from_parts(
		input.data + input.length - expected_suffix.length,
		expected_suffix.length);

	if (gcli_sv_eq(expected_suffix, actual_suffix))
		input.length -= expected_suffix.length;

	return input;
}

bool
gcli_sv_eq(gcli_sv this, gcli_sv that)
{
	if (this.length != that.length)
		return false;

	return strncmp(this.data, that.data, this.length) == 0;
}
