#include <gcli/curl.h>

#include <atf-c.h>

ATF_TC_WITHOUT_HEAD(simple_characters);
ATF_TC_BODY(simple_characters, tc)
{
	ATF_CHECK_STREQ(gcli_urlencode("%"), "%25");
	ATF_CHECK_STREQ(gcli_urlencode(" "), "%20");
	ATF_CHECK_STREQ(gcli_urlencode("-"), "-");
	ATF_CHECK_STREQ(gcli_urlencode("_"), "_");
}

ATF_TC_WITHOUT_HEAD(umlaute);
ATF_TC_BODY(umlaute, tc)
{
	ATF_CHECK_STREQ(gcli_urlencode("Ä"), "%C3%84");
	ATF_CHECK_STREQ(gcli_urlencode("ä"), "%C3%A4");
	ATF_CHECK_STREQ(gcli_urlencode("Ö"), "%C3%96");
	ATF_CHECK_STREQ(gcli_urlencode("ö"), "%C3%B6");
	ATF_CHECK_STREQ(gcli_urlencode("Ü"), "%C3%9C");
	ATF_CHECK_STREQ(gcli_urlencode("ü"), "%C3%BC");
	ATF_CHECK_STREQ(gcli_urlencode("ẞ"), "%E1%BA%9E");
	ATF_CHECK_STREQ(gcli_urlencode("ß"), "%C3%9F");
}

ATF_TC_WITHOUT_HEAD(torture);
ATF_TC_BODY(torture, tc)
{
	char text[] = "some-random url// with %%%%%content"
		"Rindfleischettikettierungsüberwachungsaufgabenübertragungsgesetz";
	char *escaped = gcli_urlencode(text);
	char *expected = "some-random%20url%2F%2F%20with%20%25%25%25%25%25content"
		"Rindfleischettikettierungs%C3%BCberwachungsaufgaben%C3%BCbertragungsgesetz";

	ATF_CHECK_STREQ(escaped, expected);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, simple_characters);
	ATF_TP_ADD_TC(tp, umlaute);
	ATF_TP_ADD_TC(tp, torture);

	return atf_no_error();
}
