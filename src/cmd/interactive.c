/*
 * Copyright 2024 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/cmd/interactive.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/** Prompt for input with an optional default
 *
 * This function prompts for user input. The prompt can be specified using a
 * format string. An optional default value can be specified. If the default
 * value is NULL the user will be repeatedly prompted until the input
 * is non-empty. */
char *
gcli_cmd_prompt(char const *const fmt, char const *const deflt, ...)
{
	char buf[256] = {0}; /* nobody types more than 256 characters, amirite? */
	va_list vp;

ask_again:
	va_start(vp, deflt);
	vfprintf(stdout, fmt, vp);
	va_end(vp);

	if (deflt)
		fprintf(stdout, " [%s]: ", deflt);
	else
		fprintf(stdout, ": ");

	fflush(stdout);

	fgets(buf, sizeof buf, stdin);

	if (buf[0] == '\n') {
		if (deflt)
			return strdup(deflt);

		memset(buf, 0, sizeof(buf));
		goto ask_again;
	}

	buf[strlen(buf)-1] = '\0';

	return strdup(buf);
}
