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

#include "unit.h"

#include <gcli/cmd/vcs/gitconf_parser.h>

#include <string.h>

/* hack to avoid linking against a gazillion of other objects */
int
gcli_vcs_guess_forgetype_by_hostname(char const *host, gcli_forge_type *out)
{
	(void) host;
	(void) out;
	return 0;
}

DEFINE_TESTCASE(tokens)
{
	char const *input = "[branch \"banana\"]\n\tremote = origin\n";
	struct gcli_gitconf_parser p = { .head = strdup(input) };

	CHECK_EQ(gcli_gitconf_parser_next_token(&p), GCLI_GITCONF_TOKEN_OBRACK);

	CHECK_EQ(gcli_gitconf_parser_next_token(&p), GCLI_GITCONF_TOKEN_LITERAL);
	CHECK_STREQ(p.token_text, "branch");

	CHECK_EQ(gcli_gitconf_parser_next_token(&p), GCLI_GITCONF_TOKEN_LITERAL);
	CHECK_STREQ(p.token_text, "banana");

	CHECK_EQ(gcli_gitconf_parser_next_token(&p), GCLI_GITCONF_TOKEN_CBRACK);


	CHECK_EQ(gcli_gitconf_parser_next_token(&p), GCLI_GITCONF_TOKEN_LITERAL);
	CHECK_STREQ(p.token_text, "remote");

	CHECK_EQ(gcli_gitconf_parser_next_token(&p), GCLI_GITCONF_TOKEN_EQUALS);

	CHECK_EQ(gcli_gitconf_parser_next_token(&p), GCLI_GITCONF_TOKEN_LITERAL);
	CHECK_STREQ(p.token_text, "origin");

	CHECK_EQ(gcli_gitconf_parser_next_token(&p), GCLI_GITCONF_TOKEN_EOF);
}

DEFINE_TESTCASE(parse_only_unknown_sections)
{
	char const *input = "[foo \"banana\"]\n\tremote = origin\n";
	struct gcli_gitconf_parser p = { .head = strdup(input) };
	struct gcli_cmd_vcs_ctx ctx = {0};

	CHECK_EQ(gcli_gitconf_parser_run(&p, &ctx), 0);
}

DEFINE_TESTCASE(one_branch)
{
	char const *input = "[branch \"trunk\"]\n\tremote = origin\n";
	struct gcli_gitconf_parser p = { .head = strdup(input) };
	struct gcli_cmd_vcs_ctx ctx = {0};
	struct gcli_cmd_vcs_branch *br;

	REQUIRE(gcli_gitconf_parser_run(&p, &ctx) == 0);
	REQUIRE(!TAILQ_EMPTY(&ctx.branches));

	br = TAILQ_FIRST(&ctx.branches);
	REQUIRE(br != NULL);

	CHECK_STREQ(br->name, "trunk");
	CHECK_STREQ(br->remote, "origin");
}

DEFINE_TESTCASE(one_remote)
{
	char const *input = "[remote \"gitlab\"]\n\turl = ssh://git@gitlab.com:herrhotzenplotz/gcli\n";
	struct gcli_gitconf_parser p = { .head = strdup(input) };
	struct gcli_cmd_vcs_ctx ctx = {0};
	struct gcli_cmd_vcs_remote *r;

	REQUIRE(gcli_gitconf_parser_run(&p, &ctx) == 0);
	REQUIRE(!TAILQ_EMPTY(&ctx.remotes));

	r = TAILQ_FIRST(&ctx.remotes);
	REQUIRE(r != NULL);

	CHECK_STREQ(r->name, "gitlab");
	CHECK_STREQ(r->owner, "herrhotzenplotz");
	CHECK_STREQ(r->repo, "gcli");
	CHECK_STREQ(r->host, "gitlab.com");
}

TESTSUITE
{
	TESTCASE(tokens);
	TESTCASE(parse_only_unknown_sections);
	TESTCASE(one_branch);
	TESTCASE(one_remote);
}
