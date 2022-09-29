#include <stdio.h>
#include <stdlib.h>

#include <atf-c.h>

#include <gcli/curl.h>

/*
 * Basic symbol tests
 */
ATF_TC(basic_symbols);
ATF_TC_HEAD(basic_symbols, tc)
{
    atf_tc_set_md_var(tc, "descr", "Basic URL Escape symbols");
}
ATF_TC_BODY(basic_symbols, tc)
{
    ATF_CHECK_STREQ(gcli_urlencode("%"), "%25");
    ATF_CHECK_STREQ(gcli_urlencode(" "), "%20");
    ATF_CHECK_STREQ(gcli_urlencode("ä"), "%C3%A4");
    ATF_CHECK_STREQ(gcli_urlencode("ẞ"), "%E1%BA%9E");
}

/*
 * Exception tests
 */
ATF_TC(exceptions);
ATF_TC_HEAD(exceptions, tc)
{
    atf_tc_set_md_var(tc, "descr", "Symbols that may not be escaped");
}
ATF_TC_BODY(exceptions, tc)
{
    ATF_CHECK_STREQ(gcli_urlencode("-"), "-");
    ATF_CHECK_STREQ(gcli_urlencode("_"), "_");
}

/*
 * Torture tests
 */
ATF_TC(verylong);
ATF_TC_HEAD(verylong, tc)
{
    atf_tc_set_md_var(tc, "descr", "Very long string to be escaped");
}
ATF_TC_BODY(verylong, tc)
{

    char  text[]  = "some-random url// with %%%%%content"
        "Rindfleischettikettierungsüberwachungsaufgabenübertragungsgesetz";
    char *escaped = gcli_urlencode(text);

    ATF_CHECK_STREQ(
        escaped,
        "some-random%20url%2F%2F%20with%20%25%25%25%25%25contentRindfleisch"
        "ettikettierungs%C3%BCberwachungsaufgaben%C3%BCbertragungsgesetz");
}


ATF_TP_ADD_TCS(tp)
{
    ATF_TP_ADD_TC(tp, basic_symbols);
    ATF_TP_ADD_TC(tp, exceptions);
    ATF_TP_ADD_TC(tp, verylong);

    return atf_no_error();
}
