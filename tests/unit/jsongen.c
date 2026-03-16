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

#include <gcli/json_gen.h>

#include <stdlib.h>
#include "unit.h"

DEFINE_TESTCASE(empty_object)
{
	struct gcli_jsongen gen = {0};

	REQUIRE(gcli_jsongen_init(&gen) == 0);
	REQUIRE(gcli_jsongen_begin_object(&gen) == 0);
	REQUIRE(gcli_jsongen_end_object(&gen) == 0);

	char *str = gcli_jsongen_to_string(&gen);
	CHECK_STREQ(str, "{}");
	free(str);

	gcli_jsongen_free(&gen);
}

DEFINE_TESTCASE(array_with_two_empty_objects)
{
	struct gcli_jsongen gen = {0};

	REQUIRE(gcli_jsongen_init(&gen) == 0);
	REQUIRE(gcli_jsongen_begin_array(&gen) == 0);
	for (int i = 0; i < 2; ++i) {
		REQUIRE(gcli_jsongen_begin_object(&gen) == 0);
		REQUIRE(gcli_jsongen_end_object(&gen) == 0);
	}
	REQUIRE(gcli_jsongen_end_array(&gen) == 0);

	char *str = gcli_jsongen_to_string(&gen);
	CHECK_STREQ(str, "[{}, {}]");
	free(str);

	gcli_jsongen_free(&gen);
}

DEFINE_TESTCASE(empty_array)
{
	struct gcli_jsongen gen = {0};

	REQUIRE(gcli_jsongen_init(&gen) == 0);
	REQUIRE(gcli_jsongen_begin_array(&gen) == 0);
	REQUIRE(gcli_jsongen_end_array(&gen) == 0);

	char *str = gcli_jsongen_to_string(&gen);
	CHECK_STREQ(str, "[]");
	free(str);

	gcli_jsongen_free(&gen);
}

DEFINE_TESTCASE(object_with_number)
{
	struct gcli_jsongen gen = {0};

	REQUIRE(gcli_jsongen_init(&gen) == 0);
	REQUIRE(gcli_jsongen_begin_object(&gen) == 0);
	REQUIRE(gcli_jsongen_objmember(&gen, "number") == 0);
	REQUIRE(gcli_jsongen_number(&gen, 420) == 0);
	REQUIRE(gcli_jsongen_end_object(&gen) == 0);

	char *str = gcli_jsongen_to_string(&gen);
	CHECK_STREQ(str, "{\"number\": 420}");
	free(str);

	gcli_jsongen_free(&gen);
}

DEFINE_TESTCASE(object_nested)
{
	struct gcli_jsongen gen = {0};

	REQUIRE(gcli_jsongen_init(&gen) == 0);
	REQUIRE(gcli_jsongen_begin_object(&gen) == 0);
	REQUIRE(gcli_jsongen_objmember(&gen, "hiernenarray") == 0);
	REQUIRE(gcli_jsongen_begin_array(&gen) == 0);
	REQUIRE(gcli_jsongen_number(&gen, 69) == 0);
	REQUIRE(gcli_jsongen_number(&gen, 420) == 0);
	REQUIRE(gcli_jsongen_end_array(&gen) == 0);
	REQUIRE(gcli_jsongen_objmember(&gen, "empty_object") == 0);
	REQUIRE(gcli_jsongen_begin_object(&gen) == 0);
	REQUIRE(gcli_jsongen_end_object(&gen) == 0);
	REQUIRE(gcli_jsongen_end_object(&gen) == 0);

	char *str = gcli_jsongen_to_string(&gen);
	CHECK_STREQ(str, "{\"hiernenarray\": [69, 420], \"empty_object\": {}}");
	free(str);

	gcli_jsongen_free(&gen);
}


DEFINE_TESTCASE(object_with_strings)
{
	struct gcli_jsongen gen = {0};

	REQUIRE(gcli_jsongen_init(&gen) == 0);
	REQUIRE(gcli_jsongen_begin_object(&gen) == 0);
	REQUIRE(gcli_jsongen_objmember(&gen, "key") == 0);
	REQUIRE(gcli_jsongen_string(&gen, "value") == 0);
	REQUIRE(gcli_jsongen_end_object(&gen) == 0);

	char *str = gcli_jsongen_to_string(&gen);
	CHECK_STREQ(str, "{\"key\": \"value\"}");
	free(str);

	gcli_jsongen_free(&gen);
}

DEFINE_TESTCASE(object_with_mixed_values)
{
	struct gcli_jsongen gen = {0};

	REQUIRE(gcli_jsongen_init(&gen) == 0);
	REQUIRE(gcli_jsongen_begin_object(&gen) == 0);
		REQUIRE(gcli_jsongen_objmember(&gen, "array") == 0);
		REQUIRE(gcli_jsongen_begin_array(&gen) == 0);

			REQUIRE(gcli_jsongen_number(&gen, 42) == 0);
			REQUIRE(gcli_jsongen_string(&gen, "a string literal") == 0);
			REQUIRE(gcli_jsongen_begin_object(&gen) == 0);
			REQUIRE(gcli_jsongen_end_object(&gen) == 0);

		REQUIRE(gcli_jsongen_end_array(&gen) == 0);
	REQUIRE(gcli_jsongen_end_object(&gen) == 0);

	char *str = gcli_jsongen_to_string(&gen);
	CHECK_STREQ(str, "{\"array\": [42, \"a string literal\", {}]}");
	free(str);

	gcli_jsongen_free(&gen);
}

DEFINE_TESTCASE(object_with_two_keys_and_values_that_are_string)
{
	struct gcli_jsongen gen = {0};

	REQUIRE(gcli_jsongen_init(&gen) == 0);
	REQUIRE(gcli_jsongen_begin_object(&gen) == 0);
		REQUIRE(gcli_jsongen_objmember(&gen, "key1") == 0);
		REQUIRE(gcli_jsongen_string(&gen, "value1") == 0);

		REQUIRE(gcli_jsongen_objmember(&gen, "key2") == 0);
		REQUIRE(gcli_jsongen_string(&gen, "value2") == 0);
	REQUIRE(gcli_jsongen_end_object(&gen) == 0);

	char *str = gcli_jsongen_to_string(&gen);
	CHECK_STREQ(str, "{\"key1\": \"value1\", \"key2\": \"value2\"}");
	free(str);

	gcli_jsongen_free(&gen);
}

TESTSUITE
{
	TESTCASE(empty_object);
	TESTCASE(array_with_two_empty_objects);
	TESTCASE(empty_array);
	TESTCASE(object_with_number);
	TESTCASE(object_nested);
	TESTCASE(object_with_strings);
	TESTCASE(object_with_mixed_values);
	TESTCASE(object_with_two_keys_and_values_that_are_string);
}
