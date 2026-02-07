/* This file is part of gcli.
 *
 * Copyright Nico Sonack <nsonack@herrhotzenplotz.de> */

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

TESTSUITE
{
	TESTCASE(tokens);
	TESTCASE(parse_only_unknown_sections);
	TESTCASE(one_branch);
}
