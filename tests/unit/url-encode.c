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

#include <gcli/curl.h>

#include "unit.h"

DEFINE_TESTCASE(simple_characters)
{
	CHECK_STREQ(gcli_urlencode("%"), "%25");
	CHECK_STREQ(gcli_urlencode(" "), "%20");
	CHECK_STREQ(gcli_urlencode("-"), "-");
	CHECK_STREQ(gcli_urlencode("_"), "_");
}

DEFINE_TESTCASE(umlaute)
{
	CHECK_STREQ(gcli_urlencode("Ä"), "%C3%84");
	CHECK_STREQ(gcli_urlencode("ä"), "%C3%A4");
	CHECK_STREQ(gcli_urlencode("Ö"), "%C3%96");
	CHECK_STREQ(gcli_urlencode("ö"), "%C3%B6");
	CHECK_STREQ(gcli_urlencode("Ü"), "%C3%9C");
	CHECK_STREQ(gcli_urlencode("ü"), "%C3%BC");
	CHECK_STREQ(gcli_urlencode("ẞ"), "%E1%BA%9E");
	CHECK_STREQ(gcli_urlencode("ß"), "%C3%9F");
}

DEFINE_TESTCASE(torture)
{
	char text[] = "some-random url// with %%%%%content"
		"Rindfleischettikettierungsüberwachungsaufgabenübertragungsgesetz";
	char *escaped = gcli_urlencode(text);
	char *expected = "some-random%20url%2F%2F%20with%20%25%25%25%25%25content"
		"Rindfleischettikettierungs%C3%BCberwachungsaufgaben%C3%BCbertragungsgesetz";

	CHECK_STREQ(escaped, expected);
}

TESTSUITE
{
	TESTCASE(simple_characters);
	TESTCASE(umlaute);
	TESTCASE(torture);
}
