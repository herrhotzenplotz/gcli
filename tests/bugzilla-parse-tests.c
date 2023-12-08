/*
 * Copyright 2023 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <err.h>
#include <string.h>

#include <gcli/gcli.h>
#include <gcli/issues.h>
#include <gcli/ctx.h>

#include <pdjson/pdjson.h>

#include "gcli_tests.h"

static gcli_forge_type
get_bugzilla_forge_type(gcli_ctx *ctx)
{
	(void) ctx;
	return GCLI_FORGE_BUGZILLA;
}

static gcli_ctx *
test_context(void)
{
	gcli_ctx *ctx;
	ATF_REQUIRE(gcli_init(&ctx, get_bugzilla_forge_type, NULL, NULL) == NULL);
	return ctx;
}

static FILE *
open_sample(char const *const name)
{
	FILE *r;
	char p[4096] = {0};

	snprintf(p, sizeof p, "%s/samples/%s", TESTSRCDIR, name);

	r = fopen(p, "r");
	return r;
}

ATF_TC_WITHOUT_HEAD(simple_bugzilla_issue);
ATF_TC_BODY(simple_bugzilla_issue, tc)
{
	gcli_issue_list list = {0};
	FILE *f;
	json_stream stream;
	gcli_ctx *ctx = test_context();

	ATF_REQUIRE(f = open_sample("bugzilla_simple_bug.json"));
	json_open_stream(&stream, f);

	json_close(&stream);
	gcli_destroy(&ctx);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, simple_bugzilla_issue);

	return atf_no_error();
}
