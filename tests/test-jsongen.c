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

#include <atf-c.h>

#include <gcli/json_gen.h>

ATF_TC_WITHOUT_HEAD(empty_object);
ATF_TC_BODY(empty_object, tc)
{
	gcli_jsongen gen = {0};

	ATF_REQUIRE(gcli_jsongen_init(&gen) == 0);
	ATF_REQUIRE(gcli_jsongen_begin_object(&gen) == 0);
	ATF_REQUIRE(gcli_jsongen_end_object(&gen) == 0);

	ATF_CHECK_STREQ(gcli_jsongen_to_string(&gen), "{}");

	gcli_jsongen_free(&gen);
}

ATF_TC_WITHOUT_HEAD(array_with_two_empty_objects);
ATF_TC_BODY(array_with_two_empty_objects, tc)
{
	gcli_jsongen gen = {0};

	ATF_REQUIRE(gcli_jsongen_init(&gen) == 0);
	ATF_REQUIRE(gcli_jsongen_begin_array(&gen) == 0);
	for (int i = 0; i < 2; ++i) {
		ATF_REQUIRE(gcli_jsongen_begin_object(&gen) == 0);
		ATF_REQUIRE(gcli_jsongen_end_object(&gen) == 0);
	}
	ATF_REQUIRE(gcli_jsongen_end_array(&gen) == 0);

	ATF_CHECK_STREQ(gcli_jsongen_to_string(&gen), "[{}, {}]");

	gcli_jsongen_free(&gen);
}

ATF_TC_WITHOUT_HEAD(empty_array);
ATF_TC_BODY(empty_array, tc)
{
	gcli_jsongen gen = {0};

	ATF_REQUIRE(gcli_jsongen_init(&gen) == 0);
	ATF_REQUIRE(gcli_jsongen_begin_array(&gen) == 0);
	ATF_REQUIRE(gcli_jsongen_end_array(&gen) == 0);

	ATF_CHECK_STREQ(gcli_jsongen_to_string(&gen), "[]");

	gcli_jsongen_free(&gen);
}

ATF_TC_WITHOUT_HEAD(object_with_number);
ATF_TC_BODY(object_with_number, tc)
{
	gcli_jsongen gen = {0};

	ATF_REQUIRE(gcli_jsongen_init(&gen) == 0);
	ATF_REQUIRE(gcli_jsongen_begin_object(&gen) == 0);
	ATF_REQUIRE(gcli_jsongen_objmember(&gen, "number") == 0);
	ATF_REQUIRE(gcli_jsongen_number(&gen, 420) == 0);
	ATF_REQUIRE(gcli_jsongen_end_object(&gen) == 0);

	ATF_CHECK_STREQ(gcli_jsongen_to_string(&gen),
	                "{\"number\": 420}");

	gcli_jsongen_free(&gen);
}

ATF_TC_WITHOUT_HEAD(object_nested);
ATF_TC_BODY(object_nested, tc)
{
	gcli_jsongen gen = {0};

	ATF_REQUIRE(gcli_jsongen_init(&gen) == 0);
	ATF_REQUIRE(gcli_jsongen_begin_object(&gen) == 0);
	ATF_REQUIRE(gcli_jsongen_objmember(&gen, "hiernenarray") == 0);
	ATF_REQUIRE(gcli_jsongen_begin_array(&gen) == 0);
	ATF_REQUIRE(gcli_jsongen_number(&gen, 69) == 0);
	ATF_REQUIRE(gcli_jsongen_number(&gen, 420) == 0);
	ATF_REQUIRE(gcli_jsongen_end_array(&gen) == 0);
	ATF_REQUIRE(gcli_jsongen_objmember(&gen, "empty_object") == 0);
	ATF_REQUIRE(gcli_jsongen_begin_object(&gen) == 0);
	ATF_REQUIRE(gcli_jsongen_end_object(&gen) == 0);
	ATF_REQUIRE(gcli_jsongen_end_object(&gen) == 0);

	ATF_CHECK_STREQ(gcli_jsongen_to_string(&gen),
	                "{\"hiernenarray\": [69, 420], \"empty_object\": {}}");

	gcli_jsongen_free(&gen);
}


ATF_TC_WITHOUT_HEAD(object_with_strings);
ATF_TC_BODY(object_with_strings, tc)
{
	gcli_jsongen gen = {0};

	ATF_REQUIRE(gcli_jsongen_init(&gen) == 0);
	ATF_REQUIRE(gcli_jsongen_begin_object(&gen) == 0);
	ATF_REQUIRE(gcli_jsongen_objmember(&gen, "key") == 0);
	ATF_REQUIRE(gcli_jsongen_string(&gen, "value") == 0);
	ATF_REQUIRE(gcli_jsongen_end_object(&gen) == 0);

	ATF_CHECK_STREQ(gcli_jsongen_to_string(&gen),
	                "{\"key\": \"value\"}");

	gcli_jsongen_free(&gen);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, empty_object);
	ATF_TP_ADD_TC(tp, array_with_two_empty_objects);
	ATF_TP_ADD_TC(tp, empty_array);
	ATF_TP_ADD_TC(tp, object_with_number);
	ATF_TP_ADD_TC(tp, object_nested);
	ATF_TP_ADD_TC(tp, object_with_strings);

	return atf_no_error();
}
