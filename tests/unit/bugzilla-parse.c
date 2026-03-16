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

#include <stdio.h>
#include <string.h>

#include <gcli/gcli.h>
#include <gcli/issues.h>
#include <gcli/ctx.h>

#include <templates/bugzilla/bugs.h>

#include <pdjson.h>

#include "unit.h"

static gcli_forge_type
get_bugzilla_forge_type(struct gcli_ctx *ctx)
{
	(void) ctx;
	return GCLI_FORGE_BUGZILLA;
}

static struct gcli_ctx *
test_context(UNIT_CTX)
{
	struct gcli_ctx *ctx;
	REQUIRE(gcli_init(&ctx, get_bugzilla_forge_type, NULL, NULL) == NULL);
	return ctx;
}

static FILE *
open_sample(char const *const name)
{
	FILE *r;
	char p[4096] = {0};

	snprintf(p, sizeof p, "%s/unit/samples/%s", TESTSRCDIR, name);

	r = fopen(p, "r");
	return r;
}

DEFINE_TESTCASE(simple_bugzilla_issue)
{
	struct gcli_issue_list list = {0};
	struct gcli_issue const *issue;
	FILE *f;
	struct json_stream stream;
	struct gcli_ctx *ctx = test_context(UNIT_CTX_VAR);

	REQUIRE((f = open_sample("bugzilla_simple_bug.json")) != NULL);
	json_open_stream(&stream, f);

	REQUIRE(parse_bugzilla_bugs(ctx, &stream, &list) == 0);

	REQUIRE(list.issues_size == 1);

	issue = &list.issues[0];

	CHECK_EQ(issue->number, 1);
	CHECK_STREQ(issue->title, "[aha] [scsi] Toshiba MK156FB scsi drive does not work with 2.0 kernel");
	CHECK_EQ(issue->created_at, 779533801);
	CHECK_STREQ(issue->author, "Dave Evans");
	CHECK_STREQ(issue->state, "Closed");
	CHECK_STREQ(issue->product, "Base System");
	CHECK_STREQ(issue->component, "kern");

	json_close(&stream);
	gcli_destroy(&ctx);
}

DEFINE_TESTCASE(bugzilla_comments)
{
	FILE *f;
	struct gcli_comment const *cmt = NULL;
	struct gcli_comment_list list = {0};
	struct gcli_ctx *ctx = test_context(UNIT_CTX_VAR);
	struct json_stream stream;

	REQUIRE((f = open_sample("bugzilla_comments.json")) != NULL);
	json_open_stream(&stream, f);

	REQUIRE(parse_bugzilla_comments(ctx, &stream, &list) == 0);
	json_close(&stream);
	fclose(f);
	f = NULL;

	REQUIRE(list.comments_size == 1);

	cmt = &list.comments[0];
	CHECK_EQ(cmt->id, 1285943);
	CHECK_STREQ(cmt->author, "zlei@FreeBSD.org");
	CHECK_EQ(cmt->date, 1701105615);
	CHECK(cmt->body != NULL);

	gcli_comments_free(&list);
	gcli_destroy(&ctx);
}

DEFINE_TESTCASE(bugzilla_attachments)
{
	FILE *f = NULL;
	struct gcli_attachment const *it;
	struct gcli_attachment_list list = {0};
	struct gcli_ctx *ctx = test_context(UNIT_CTX_VAR);
	struct json_stream stream = {0};

	REQUIRE((f = open_sample("bugzilla_attachments.json")) != NULL);
	json_open_stream(&stream, f);

	REQUIRE(parse_bugzilla_bug_attachments(ctx, &stream, &list) == 0);

	CHECK(list.attachments_size == 2);

	it = list.attachments;
	CHECK_EQ(it->id, 246131);
	CHECK_EQ(it->is_obsolete, true);
	CHECK_STREQ(it->author, "nsonack@outlook.com");
	CHECK_STREQ(it->content_type, "text/plain");
	CHECK_EQ(it->created_at, 1699129151);
	CHECK_STREQ(it->file_name, "0001-devel-open62541-Update-to-version-1.3.8.patch");
	CHECK_STREQ(it->summary, "Patch for updating the port");

	it++;
	CHECK_EQ(it->id, 246910);
	CHECK_EQ(it->is_obsolete, false);
	CHECK_STREQ(it->author, "nsonack@outlook.com");
	CHECK_STREQ(it->content_type, "text/plain");
	CHECK_EQ(it->created_at, 1702055406);
	CHECK_STREQ(it->file_name, "0001-devel-open62541-Update-to-version-1.3.8.patch");
	CHECK_STREQ(it->summary, "Patch v2 (now for version 1.3.9)");

	gcli_attachments_free(&list);

	json_close(&stream);
	fclose(f);
	f = NULL;

	gcli_destroy(&ctx);
}

TESTSUITE
{
	TESTCASE(simple_bugzilla_issue);
	TESTCASE(bugzilla_comments);
	TESTCASE(bugzilla_attachments);
}
