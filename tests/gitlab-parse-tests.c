#include <templates/github/issues.h>
#include <templates/github/pulls.h>
#include <templates/github/labels.h>

#include <err.h>
#include <string.h>

#include <gcli/ctx.h>

#include <atf-c.h>

#include <config.h>

static gcli_forge_type
get_gitlab_forge_type(gcli_ctx *ctx)
{
	(void) ctx;
	return GCLI_FORGE_GITLAB;
}

static gcli_ctx *
test_context(void)
{
	gcli_ctx *ctx;
	ATF_REQUIRE(gcli_init(&ctx, get_gitlab_forge_type, NULL, NULL) == NULL);
	return ctx;
}

static FILE *
open_sample(char const *const name)
{
	FILE *r;
	char p[4096] = {0};

	snprintf(p, sizeof p, "%s/samples/%s", TESTSRCDIR, name);

	ATF_REQUIRE(r = fopen(p, "r"));

	return r;
}

ATF_TC_WITHOUT_HEAD(gitlab_simple_merge_request);
ATF_TC_BODY(gitlab_simple_merge_request, tc)
{
	gcli_ctx *ctx = test_context();
	FILE *f = open_sample("gitlab_simple_merge_request.json");

	(void) ctx;
	(void) f;
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, gitlab_simple_merge_request);
	return atf_no_error();
}
