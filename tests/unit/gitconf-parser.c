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

TESTSUITE
{
	TESTCASE(tokens);
}
