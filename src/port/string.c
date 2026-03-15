/*
 * Copyright Nico Sonack <nsonack@herrhotzenplotz.de>
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
#include <gcli/port/err.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

char *
gcli_vasprintf(char const *const fmt, va_list vp)
{
	char tmp = 0, *result = NULL;
	size_t actual = 0;
	va_list vp_copy;

	va_copy(vp_copy, vp);

	actual = vsnprintf(&tmp, 1, fmt, vp_copy);

	result = calloc(1, actual + 1);
	if (!result)
		err(1, "calloc");

	vsnprintf(result, actual + 1, fmt, vp);

	return result;
}

char *
gcli_asprintf(const char *const fmt, ...)
{
	char *result;
	va_list vp;

	va_start(vp, fmt);
	result = gcli_vasprintf(fmt, vp);
	va_end(vp);

	return result;
}

char *
gcli_strndup(const char *it, size_t len)
{
	char *result = NULL;
	char const *tmp = NULL;
	size_t actual = 0;

	if (!len)
		return NULL;

	tmp = it;

	while (tmp[actual++] && actual < len);

	result = calloc(1, actual + 1);
	memcpy(result, it, actual);
	return result;
}

char *
gcli_join_with(char const *const items[], size_t const items_size, char const *sep)
{
	char *buffer = NULL;
	size_t buffer_size = 0;
	size_t bufoff = 0;
	size_t sep_size = 0;

	sep_size = strlen(sep);

	/* this works because of the null terminator at the end */
	for (size_t i = 0; i < items_size; ++i) {
		buffer_size += strlen(items[i]) + sep_size;
	}

	buffer = calloc(1, buffer_size);
	if (!buffer)
		return NULL;

	for (size_t i = 0; i < items_size; ++i) {
		size_t len = strlen(items[i]);

		memcpy(buffer + bufoff, items[i], len);
		if (i != items_size - 1)
			memcpy(&buffer[bufoff + len], sep, sep_size);

		bufoff += len + sep_size;
	}

	return buffer;
}
