/*
 * Copyright 2025 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/gcli.h>
#include <gcli/curl.h>
#include <gcli/url.h>

#include <gcli/port/string.h>

#include <string.h>

int
gcli_parse_url(char const *const input, struct gcli_url *out)
{
	size_t n;
	char *buf, *hd, *tmp;

	hd = buf = strdup(input);

	/* check if we can find a scheme */
	tmp = strstr(hd, "://");
	if (tmp) {
		out->scheme = gcli_strndup(hd, tmp - hd);
		hd = tmp + 3;
	}

	/* now an optional user, terminated by an '@' */
	tmp = strchr(hd, '@');
	if (tmp) {
		out->user = gcli_strndup(hd, tmp - hd);
		hd = tmp + 1;
	}

	/* now the host, terminated by either a ':' or a '/'
	 *
	 * If not found, the entire thing is the host. */
	tmp = strpbrk(hd, ":/");
	if (!tmp) {
		out->host = strdup(hd);
		gcli_clear_ptr(&buf);
		return 0;
	}

	out->host = gcli_strndup(hd, tmp - hd);
	hd = tmp;

	/* if we found a ':', try and parse a port number! */
	if (*hd++ == ':') {
		n = strspn(hd, "0123456789");
		if (n)
			out->port = gcli_strndup(hd, n);

		/* skip over parsed characters */
		hd += n;

		/* we should point at a '/' now */
		if (*hd == '/')
			hd += 1;
	}

	out->path = strdup(hd);
	gcli_clear_ptr(&buf);

	return 0;
}

void
gcli_url_free(struct gcli_url *url)
{
	gcli_clear_ptr(&url->scheme);
	gcli_clear_ptr(&url->user);
	gcli_clear_ptr(&url->host);
	gcli_clear_ptr(&url->port);
	gcli_clear_ptr(&url->path);
}

void
gcli_url_options_append(char **result, char const *const key,
                        char const *const value)
{
	char *e_key, *e_value, *kvp;
	size_t kvplen, resultlen;

	if (key == NULL || value == NULL)
		return;

	e_key = gcli_urlencode(key);
	e_value = gcli_urlencode(value);

	kvp = gcli_asprintf("%c%s=%s", *result ? '&' : '?', e_key, e_value);

	gcli_clear_ptr(&e_key);
	gcli_clear_ptr(&e_value);

	kvplen = strlen(kvp);

	resultlen = 0;
	if (*result)
		resultlen = strlen(*result);

	*result = realloc(*result, kvplen + resultlen + 1);
	memcpy(*result + resultlen, kvp, kvplen + 1);

	gcli_clear_ptr(&kvp);
}

void
gcli_url_options_appendf(char **result, char const *const key,
                         char const *const fmt, ...)
{
	char *value;
	va_list vp;

	if (key == NULL || fmt == NULL)
		return;

	va_start(vp, fmt);
	value = gcli_vasprintf(fmt, vp);
	va_end(vp);

	gcli_url_options_append(result, key, value);
	gcli_clear_ptr(&value);
}
