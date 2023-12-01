#include <gcli/json_util.h>

#include <atf-c.h>

ATF_TC_WITHOUT_HEAD(newlines);
ATF_TC_BODY(newlines, tc)
{
	sn_sv const input = SV("\n\r");
	sn_sv const escaped = gcli_json_escape(input);

	ATF_CHECK(sn_sv_eq_to(escaped, "\\n\\r"));
	free(escaped.data);
}

ATF_TC_WITHOUT_HEAD(tabs);
ATF_TC_BODY(tabs, tc)
{
	sn_sv const input = SV("\t\t\t");
	sn_sv const escaped = gcli_json_escape(input);

	ATF_CHECK(sn_sv_eq_to(escaped, "\\t\\t\\t"));
	free(escaped.data);
}

ATF_TC_WITHOUT_HEAD(backslashes);
ATF_TC_BODY(backslashes, tc)
{
	sn_sv const input = SV("\\");
	sn_sv const escaped = gcli_json_escape(input);

	ATF_CHECK(sn_sv_eq_to(escaped, "\\\\"));
	free(escaped.data);
}

ATF_TC_WITHOUT_HEAD(torture);
ATF_TC_BODY(torture, tc)
{
	sn_sv const input = SV("\n\r\n\n\n\t{}");
	sn_sv const escaped = gcli_json_escape(input);

	ATF_CHECK(sn_sv_eq_to(escaped, "\\n\\r\\n\\n\\n\\t{}"));
	free(escaped.data);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, newlines);
	ATF_TP_ADD_TC(tp, tabs);
	ATF_TP_ADD_TC(tp, backslashes);
	ATF_TP_ADD_TC(tp, torture);

	return atf_no_error();
}
