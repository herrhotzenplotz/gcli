#include <string.h>

#include <gcli/gcli.h>
#include <gcli/ctx.h>
#include <gcli/status.h>

#include <pdjson.h>

#include "unit.h"

#include <templates/gitea/status.h>

static gcli_forge_type
get_gitea_forge_type(struct gcli_ctx *ctx)
{
	(void) ctx;
	return GCLI_FORGE_GITEA;
}

static struct gcli_ctx *
test_context(UNIT_CTX)
{
	struct gcli_ctx *ctx;
	REQUIRE(gcli_init(&ctx, get_gitea_forge_type, NULL, NULL) == NULL);
	return ctx;
}

static FILE *
open_sample(UNIT_CTX, char const *const name)
{
	FILE *r;
	char p[4096] = {0};

	snprintf(p, sizeof p, "%s/unit/samples/%s", TESTSRCDIR, name);

	REQUIRE((r = fopen(p, "r")) != NULL);

	return r;
}

DEFINE_TESTCASE(gitea_simple_notification)
{
	struct gcli_notification notification = {0};
	FILE *sample;
	struct json_stream stream = {0};
	struct gcli_ctx *ctx;

	ctx = test_context(UNIT_CTX_VAR);
	sample = open_sample(_ctx, "gitea_simple_notification.json");

	json_open_stream(&stream, sample);
	REQUIRE(parse_gitea_notification(ctx, &stream, &notification) == 0);

	CHECK_STREQ(notification.id, "511579");
	CHECK_STREQ(notification.title, "Remove register from C++ sources");
	CHECK(notification.reason == NULL);
	CHECK_STREQ(notification.date, "2023-11-24T21:01:50Z");
	CHECK_STREQ(notification.repository, "schilytools/schilytools");

	fclose(sample);
	gcli_free_notification(&notification);
	gcli_destroy(&ctx);
}

TESTSUITE
{
	TESTCASE(gitea_simple_notification);
}
