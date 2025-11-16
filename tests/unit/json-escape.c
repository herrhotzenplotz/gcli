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
