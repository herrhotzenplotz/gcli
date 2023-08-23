/*
 * Copyright 2023 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/date_time.h>

#include <gcli/gcli.h>

#include <assert.h>
#include <time.h>

int
gcli_normalize_date(gcli_ctx *ctx, int fmt, char const *const input,
                    char *output, size_t const output_size)
{
	struct tm tm_buf = {0};
	struct tm *utm_buf;
	char *endptr;
	time_t utctime;
	char const *sfmt;

	switch (fmt) {
	case DATEFMT_ISO8601:
		sfmt = "%Y-%m-%dT%H:%M:%SZ";
		assert(output_size == 21);
		break;
	case DATEFMT_GITLAB:
		sfmt = "%Y%m%d";
		assert(output_size == 9);
		break;
	default:
		return gcli_error(ctx, "bad date format");
	}

	/* Parse input time */
	endptr = strptime(input, "%Y-%m-%d", &tm_buf);
	if (endptr == NULL || *endptr != '\0')
		return gcli_error(ctx, "date »%s« is invalid: want YYYY-MM-DD", input);

	/* Convert to UTC: Really, we should be using the _r versions of
	 * these functions for thread-safety but since gcli doesn't do
	 * multithreading (except for inside libcurl) we do not need to be
	 * worried about the storage behind the pointer returned by gmtime
	 * to be altered by another thread. */
	utctime = mktime(&tm_buf);
	utm_buf = gmtime(&utctime);

	/* Format the output string - now in UTC */
	strftime(output, output_size, sfmt, utm_buf);

	return 0;
}
