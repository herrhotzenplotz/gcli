#include <stdio.h>
#include <stdlib.h>

#include <atf-c.h>

#include <gcli/json_util.h>

ATF_TC(basics);
ATF_TC_HEAD(basics, tc)
{
	atf_tc_set_md_var(tc, "descr", "basic sanity tests");
}
ATF_TC_BODY(basics, tc)
{
	ATF_REQUIRE(
		sn_sv_eq_to(
			gcli_json_escape(SV("\n\r\n\n\n\t{}")),
			"\\n\\r\\n\\n\\n\\t{}"));

	ATF_REQUIRE(
		sn_sv_eq_to(
			gcli_json_escape(SV("\\")),
			"\\\\"));
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, basics);

	return atf_no_error();
}
