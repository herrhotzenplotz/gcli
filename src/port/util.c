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

#include <gcli/port/util.h>

#include <gcli/port/sv.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

bool
gcli_yesno(const char *fmt, ...)
{
	char tmp = 0;
	va_list vp;
	gcli_sv message = {0};
	bool result = false;

	va_start(vp, fmt);
	message.length = vsnprintf(&tmp, 1, fmt, vp);
	va_end(vp);
	message.data = calloc(1, message.length + 1);

	va_start(vp, fmt);
	vsnprintf(message.data, message.length + 1, fmt, vp);
	va_end(vp);

	do {
		printf(SV_FMT" [yN] ", SV_ARGS(message));

		char c = getchar();

		if (c == 'y' || c == 'Y') {
			result = true;
			getchar();
			break;
		} else if (c == 'n' || c == 'N') {
			getchar();
			break;
		} else if (c == '\n') {
			break;
		}

		getchar(); // consume newline character

	} while (!feof(stdin));

	free(message.data);
	return result;
}

int
gcli_read_file(char const *path, char **buffer)
{
	FILE *f;
	size_t len;
	int rc = 0;

	/* open file and determine length */
	f = fopen(path, "r");
	if (!f)
		return -1;

	if (fseek(f, 0, SEEK_END) < 0)
		goto err_seek;

	len = ftell(f);
	rewind(f);

	*buffer = malloc(len + 1);
	if (fread(*buffer, 1, len, f) != len) {
		rc = -1;
		goto err_read;
	}

	(*buffer)[len] = '\0';

	rc = (int)(len);

err_read:
err_seek:
	fclose(f);

	return rc;
}
