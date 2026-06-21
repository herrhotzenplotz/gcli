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

#include <gcli/base64.h>

#include "unit.h"

DEFINE_TESTCASE(simple_decode)
{
	char const input[] = "aGVsbG8gd29ybGQ=";
	char output[sizeof("hello world")] = {0};

	int rc = gcli_decode_base64(NULL, input, output, sizeof(output));
	REQUIRE(rc == 0);
	CHECK_STREQ(output, "hello world");
}

#define ENCODE_CASE(name, in, out)                                                    \
	DEFINE_TESTCASE(name)                                                         \
	{                                                                             \
		uint8_t const input[] = in;                                           \
		char *output = NULL;                                                  \
                                                                                      \
		int rc = gcli_encode_base64(NULL, input, sizeof(input) - 1, &output); \
		REQUIRE(rc == 0);                                                     \
		CHECK_STREQ(output, out);                                             \
                                                                                      \
		free(output);                                                         \
	}

ENCODE_CASE(encode_onebyte, "A", "QQ==")
ENCODE_CASE(encode_twobyte, "A\n", "QQo=")
ENCODE_CASE(encode_threebyte, "AA\n", "QUEK")
ENCODE_CASE(encode_foobar, "foobar", "Zm9vYmFy")
ENCODE_CASE(encode_binary, "\01\02\02\04\05\06\07\010\011\012",
                           "AQICBAUGBwgJCg==")

TESTSUITE
{
	TESTCASE(simple_decode);

	TESTCASE(encode_onebyte);
	TESTCASE(encode_twobyte);
	TESTCASE(encode_threebyte);
	TESTCASE(encode_foobar);
	TESTCASE(encode_binary);
}
