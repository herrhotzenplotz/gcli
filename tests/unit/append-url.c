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

#include <gcli/url.h>

#include <stdlib.h>
#include "unit.h"

DEFINE_TESTCASE(sanity)
{
	char *options = NULL;

	gcli_url_options_append(&options, NULL, NULL);
	CHECK_EQ(options, NULL);
}

DEFINE_TESTCASE(one_option)
{
	char *options = NULL;

	gcli_url_options_append(&options, "foo", "bar");

	REQUIRE(options != NULL);
	CHECK_STREQ(options, "?foo=bar");

	free(options);
}

DEFINE_TESTCASE(two_options)
{
	char *options = NULL;

	gcli_url_options_append(&options, "foo", "bar");

	REQUIRE(options != NULL);
	CHECK_STREQ(options, "?foo=bar");

	gcli_url_options_append(&options, "baz", "banana");
	REQUIRE(options != NULL);
	CHECK_STREQ(options, "?foo=bar&baz=banana");

	free(options);
}

DEFINE_TESTCASE(three_options_with_one_null)
{
	char *options = NULL;

	gcli_url_options_append(&options, "foo", "bar");

	REQUIRE(options != NULL);
	CHECK_STREQ(options, "?foo=bar");

	gcli_url_options_append(&options, "peanut", NULL);
	REQUIRE(options != NULL);
	CHECK_STREQ(options, "?foo=bar");

	gcli_url_options_append(&options, "baz", "banana");
	REQUIRE(options != NULL);
	CHECK_STREQ(options, "?foo=bar&baz=banana");

	free(options);
}

DEFINE_TESTCASE(urlencoded_options)
{
	char *options = NULL;

	gcli_url_options_append(&options, "path", "foo/bar/baz");
	REQUIRE(options != NULL);
	CHECK_STREQ(options, "?path=foo%2Fbar%2Fbaz");

	free(options);
}

TESTSUITE
{
	TESTCASE(sanity);
	TESTCASE(one_option);
	TESTCASE(two_options);
	TESTCASE(three_options_with_one_null);
	TESTCASE(urlencoded_options);
}
