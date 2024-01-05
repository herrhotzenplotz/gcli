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

#include <gcli/base64.h>

#include <stdlib.h>
#include <string.h>

/* The code below is taken from my IRC chat bot and was originally written by
 * raym aka. Aritra Sarkar <aritra1911@yahoo.com> in 2022. */
int
gcli_decode_base64(struct gcli_ctx *ctx, char const *input, char *buffer,
                   size_t buffer_size)
{
	char const digits[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxy"
	                      "z0123456789+/";

	char digit = 0;
	unsigned long octets = 0;
	size_t bits_rem = 0;
	size_t length = 0;

	memset(buffer, 0, buffer_size);

	while ((digit = *input++)) {

		if (digit == '=') {
			if (bits_rem % 8 == 0)
				/* Unnecessary padding char */
				return gcli_error(ctx, "invalid base64 input");

			do {
				if (digit != '=')
					return gcli_error(ctx, "invalid base64 input");

				octets >>= 2;
				bits_rem -= 2;

				digit = *input++;

			} while (bits_rem % 8 != 0);

			if (digit)
				return gcli_error(ctx, "invalid base64 input");

			size_t byte_count = 0;
			while (bits_rem > 0) {
				unsigned char octet = octets & 0xff;

				if (octet == '\0')
					return gcli_error(ctx, "null-character encountered during base64 decode");

				buffer[length + (bits_rem / 8) - 1] = (char) octet;
				octets >>= 8;
				bits_rem -= 8;
				byte_count++;
			}

			length += byte_count;

			return 0;
		}

		size_t sextet = 0;

		/* Lookup index for a digit.  We shall perform a linear search
		 * for the index of the digit.  Since there are only 64 digits,
		 * this should be done in a jiffy. */
		for ( ; sextet < 64; sextet++)
			if (digits[sextet] == digit)
				break;

		if (sextet == 64)
			/* Oops!  We couldn't lookup the index of `digit` */
			return gcli_error(ctx, "invalid base64 input");

		octets = (octets << 6) | sextet;
		bits_rem += 6;

		/* 4 sextets (24 bits) of base64 input yields 3 bytes */

		if (bits_rem == 24) {
			while (bits_rem > 0) {
				unsigned char octet = octets & 0xff;

				if (octet == '\0')
					return gcli_error(ctx, "null-character encountered during base64 decode");

				buffer[length + (bits_rem / 8) - 1] = (char) octet;
				octets >>= 8;
				bits_rem -= 8;
			}

			length += 3;
		}
	}

	if (bits_rem > 0)
		return gcli_error(ctx, "invalid base64 input");

	return 0;
}

int
gcli_base64_decode_print(struct gcli_ctx *ctx, FILE *out, char const *const input)
{
	int rc = 0;
	char *buffer = NULL;
	size_t buffer_size = 0, input_size = 0;

	input_size = strlen(input);
	/* account for BASE64 inflation */
	buffer_size = (input_size / 4) * 3;
	buffer = calloc(1, buffer_size);

	rc = gcli_decode_base64(ctx, input, buffer, buffer_size);
	if (rc < 0)
		return rc;

	fwrite(buffer, buffer_size, 1, out);

	free(buffer);
	buffer = NULL;

	return 0;
}
