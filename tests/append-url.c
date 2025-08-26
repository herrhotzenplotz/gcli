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

#include <atf-c.h>

#include <gcli/url.h>

#include <stdlib.h>

ATF_TC_WITHOUT_HEAD(sanity);
ATF_TC_BODY(sanity, tc)
{
	char *options = NULL;

	gcli_url_options_append(&options, NULL, NULL);
	ATF_CHECK_EQ(options, NULL);
}

ATF_TC_WITHOUT_HEAD(one_option);
ATF_TC_BODY(one_option, tc)
{
	char *options = NULL;

	gcli_url_options_append(&options, "foo", "bar");

	ATF_REQUIRE(options != NULL);
	ATF_CHECK_STREQ(options, "?foo=bar");

	free(options);
}

ATF_TC_WITHOUT_HEAD(two_options);
ATF_TC_BODY(two_options, tc)
{
	char *options = NULL;

	gcli_url_options_append(&options, "foo", "bar");

	ATF_REQUIRE(options != NULL);
	ATF_CHECK_STREQ(options, "?foo=bar");

	gcli_url_options_append(&options, "baz", "banana");
	ATF_REQUIRE(options != NULL);
	ATF_CHECK_STREQ(options, "?foo=bar&baz=banana");

	free(options);
}

ATF_TC_WITHOUT_HEAD(three_options_with_one_null);
ATF_TC_BODY(three_options_with_one_null, tc)
{
	char *options = NULL;

	gcli_url_options_append(&options, "foo", "bar");

	ATF_REQUIRE(options != NULL);
	ATF_CHECK_STREQ(options, "?foo=bar");

	gcli_url_options_append(&options, "peanut", NULL);
	ATF_REQUIRE(options != NULL);
	ATF_CHECK_STREQ(options, "?foo=bar");

	gcli_url_options_append(&options, "baz", "banana");
	ATF_REQUIRE(options != NULL);
	ATF_CHECK_STREQ(options, "?foo=bar&baz=banana");

	free(options);
}

ATF_TC_WITHOUT_HEAD(urlencoded_options);
ATF_TC_BODY(urlencoded_options, tc)
{
	char *options = NULL;

	gcli_url_options_append(&options, "path", "foo/bar/baz");
	ATF_REQUIRE(options != NULL);
	ATF_CHECK_STREQ(options, "?path=foo%2Fbar%2Fbaz");

	free(options);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, sanity);
	ATF_TP_ADD_TC(tp, one_option);
	ATF_TP_ADD_TC(tp, two_options);
	ATF_TP_ADD_TC(tp, three_options_with_one_null);
	ATF_TP_ADD_TC(tp, urlencoded_options);

	return atf_no_error();
}
