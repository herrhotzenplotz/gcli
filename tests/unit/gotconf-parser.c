/* This file is part of gcli.
 *
 * Copyright Nico Sonack <nsonack@herrhotzenplotz.de> */

#include "unit.h"

#include <gcli/cmd/vcs/gotconf_parser.h>

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
	char const *input = "unquoted_literal \"quoted_literal\" { }";
	struct gcli_gotconf_parser p = { .head = strdup(input) };

	CHECK(gcli_gotconf_parser_next_token(&p) == GCLI_GOTCONF_TOKEN_LITERAL);
	CHECK_STREQ(p.token_text, "unquoted_literal");
	CHECK(gcli_gotconf_parser_next_token(&p) == GCLI_GOTCONF_TOKEN_LITERAL);
	CHECK_STREQ(p.token_text, "quoted_literal");
	CHECK(gcli_gotconf_parser_next_token(&p) == GCLI_GOTCONF_TOKEN_OCURLY);
	CHECK(gcli_gotconf_parser_next_token(&p) == GCLI_GOTCONF_TOKEN_CCURLY);
	CHECK(gcli_gotconf_parser_next_token(&p) == GCLI_GOTCONF_TOKEN_EOF);
}

DEFINE_TESTCASE(single_remote)
{
	char input[] =
		"remote \"origin\" {"
		"	server git@git.sr.ht"
		"	protocol ssh"
		"	repository \"~herrhotzenplotz/gcli\""
		"	branch { \"trunk\" }"
		"}";

	struct gcli_gotconf_parser p = { .head = input };
	struct gcli_cmd_vcs_remotes rs = {0};

	CHECK(gcli_gotconf_parser_run(&p, &rs) == 0);
}

TESTSUITE
{
	TESTCASE(tokens);
	TESTCASE(single_remote);
}
