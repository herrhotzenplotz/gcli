/*
 * Copyright Nico Sonack <nsonack@herrhotzenplotz.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
	REQUIRE(gcli_init(&ctx, get_gitea_forge_type,
	        (char const *(*)(struct gcli_ctx *))NULL,
	        (char const *(*)(struct gcli_ctx *))NULL) == NULL);
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
