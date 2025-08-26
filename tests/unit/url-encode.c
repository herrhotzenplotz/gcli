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
