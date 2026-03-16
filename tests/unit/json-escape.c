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

#include <gcli/json_util.h>

#include "unit.h"

DEFINE_TESTCASE(newlines)
{
	char const *input = "\n\r";
	char *escaped = gcli_json_escape(input);

	CHECK_STREQ(escaped, "\\n\\r");
	free(escaped);
}

DEFINE_TESTCASE(tabs)
{
	char const *input = "\t\t\t";
	char *escaped = gcli_json_escape(input);

	CHECK_STREQ(escaped, "\\t\\t\\t");
	free(escaped);
}

DEFINE_TESTCASE(backslashes)
{
	char const *input = "\\";
	char *escaped = gcli_json_escape(input);

	CHECK_STREQ(escaped, "\\\\");
	free(escaped);
}

DEFINE_TESTCASE(torture)
{
	char const *input = "\n\r\n\n\n\t{}";
	char *escaped = gcli_json_escape(input);

	CHECK_STREQ(escaped, "\\n\\r\\n\\n\\n\\t{}");
	free(escaped);
}

TESTSUITE
{
	TESTCASE(newlines);
	TESTCASE(tabs);
	TESTCASE(backslashes);
	TESTCASE(torture);
}
