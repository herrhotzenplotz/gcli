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

#ifndef GCLI_CMD_VCS_GOTCONFPARSER_H
#define GCLI_CMD_VCS_GOTCONFPARSER_H

#include <gcli/gcli.h>
#include <gcli/cmd/vcs.h>

struct gcli_gotconf_parser {
	int line;
	char *head;
	char *token_text;
	char *error_message;
};

enum {
	GCLI_GOTCONF_TOKEN_OCURLY = '{',
	GCLI_GOTCONF_TOKEN_CCURLY = '}',
	GCLI_GOTCONF_TOKEN_LITERAL = 1,
	GCLI_GOTCONF_TOKEN_EOF     = 0,
};

int gcli_gotconf_parser_next_token(struct gcli_gotconf_parser *p);
int gcli_gotconf_parser_run(struct gcli_gotconf_parser *p, struct gcli_cmd_vcs_remotes *out);

#endif /* GCLI_CMD_VCS_GOTCONFPARSER_H */
