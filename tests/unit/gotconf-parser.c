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
