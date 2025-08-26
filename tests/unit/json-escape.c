#include <gcli/json_util.h>

#include "unit.h"

DEFINE_TESTCASE(newlines)
{
	gcli_sv const input = SV("\n\r");
	gcli_sv const escaped = gcli_json_escape(input);

	CHECK(gcli_sv_eq_to(escaped, "\\n\\r"));
	free(escaped.data);
}

DEFINE_TESTCASE(tabs)
{
	gcli_sv const input = SV("\t\t\t");
	gcli_sv const escaped = gcli_json_escape(input);

	CHECK(gcli_sv_eq_to(escaped, "\\t\\t\\t"));
	free(escaped.data);
}

DEFINE_TESTCASE(backslashes)
{
	gcli_sv const input = SV("\\");
	gcli_sv const escaped = gcli_json_escape(input);

	CHECK(gcli_sv_eq_to(escaped, "\\\\"));
	free(escaped.data);
}

DEFINE_TESTCASE(torture)
{
	gcli_sv const input = SV("\n\r\n\n\n\t{}");
	gcli_sv const escaped = gcli_json_escape(input);

	CHECK(gcli_sv_eq_to(escaped, "\\n\\r\\n\\n\\n\\t{}"));
	free(escaped.data);
}

TESTSUITE
{
	TESTCASE(newlines);
	TESTCASE(tabs);
	TESTCASE(backslashes);
	TESTCASE(torture);
}
