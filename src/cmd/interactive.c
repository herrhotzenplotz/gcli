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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(HAVE_LIBEDIT)
#   if HAVE_LIBEDIT
#       include <histedit.h>
#       define USE_EDITLINE 1
#   else
#       define USE_EDITLINE 0
#   endif
#else
#   define USE_EDITLINE 0
#endif

#if !USE_EDITLINE && defined(HAVE_READLINE)
#   if HAVE_READLINE
#       include <readline/readline.h>
#       define USE_READLINE 1
#   else
#       define USE_READLINE 0
#   endif
#else
#   define USE_READLINE 0
#endif

#if USE_EDITLINE
static char *
el_prompt_fn(EditLine *el_ctx)
{
	char *prompt = NULL;
	if (el_get(el_ctx, EL_CLIENTDATA, &prompt) < 0)
		return strdup(">");
	else
		return prompt;
}
#endif

/* Actual implementation for reading in the line */
static char *
get_input_line(char *const prompt)
{
#if USE_EDITLINE
	static EditLine *el_ctx = NULL;
	char const *txt = NULL;
	char *result = NULL;
	int len = 0;

	if (!el_ctx) {
		el_ctx = el_init("gcli", stdin, stdout, stderr);
		el_set(el_ctx, EL_PROMPT, el_prompt_fn);
		el_set(el_ctx, EL_EDITOR, "emacs");
	}

	el_set(el_ctx, EL_CLIENTDATA, prompt);

	txt = el_gets(el_ctx, &len);
	bool const is_error = txt == NULL || len < 0;
	bool const is_empty = len == 1 && txt[0] == '\n';
	if (is_error || is_empty)
		return NULL;

	result = strdup(txt);
	result[len - 1] = '\0';

	return result;

#elif USE_READLINE
	char *result = readline(prompt);

	/* readline() returns an empty string if the input is empty. Our interface
	 * returns NULL if the input was empty */
	if (result == NULL || result[0] == '\0') {
		free(result);
		result = NULL;
	}

	return result;

#else
	char buf[256] = {0}; /* nobody types more than 256 characters, amirite? */

	fputs(prompt, stdout);
	fflush(stdout);
	fgets(buf, sizeof buf, stdin);

	if (buf[0] == '\n')
		return NULL;

	buf[strlen(buf)-1] = '\0';
	return strdup(buf);
#endif
}

/** Prompt for input with an optional default
 *
 * This function prompts for user input, possibly with editline
 * capabilities. The prompt can be specified using a format string.
 * An optional default value can be specified. If the default value
 * is NULL the user will be repeatedly prompted until the input is
 * non-empty. */
char *
gcli_cmd_prompt(char const *const fmt, char const *const deflt, ...)
{
	va_list vp;
	char *result;
	char prompt[256] = {0};
	size_t prompt_len;

	va_start(vp, deflt);
	vsnprintf(prompt, sizeof(prompt), fmt, vp);
	va_end(vp);

	prompt_len = strlen(prompt);
	if (deflt) {
		snprintf(prompt + prompt_len, sizeof(prompt) - prompt_len, " [%s]: ", deflt);
	} else {
		strncat(prompt, ": ", sizeof(prompt) - prompt_len - 1);
	}

	do {
		result = get_input_line(prompt);
	} while (deflt == NULL && result == NULL);

	if (result == NULL)
		result = strdup(deflt);

	return result;
}
