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

TESTSUITE
{
	TESTCASE(simple_decode);
}
