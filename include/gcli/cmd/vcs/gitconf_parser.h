/* This file is part of gcli.
 *
 * Copyright Nico Sonack <nsonack@herrhotzenplotz.de> */

#ifndef GCLI_CMD_VCS_GITCONFPARSER_H
#define GCLI_CMD_VCS_GITCONFPARSER_H

#include <gcli/gcli.h>
#include <gcli/cmd/vcs.h>

struct gcli_gitconf_parser {
	int line;
	char *head;
	char *token_text;
	char *error_message;
};

enum {
	GCLI_GITCONF_TOKEN_OBRACK = '[',
	GCLI_GITCONF_TOKEN_CBRACK = ']',
	GCLI_GITCONF_TOKEN_EQUALS = '=',
	GCLI_GITCONF_TOKEN_LITERAL = 1,
	GCLI_GITCONF_TOKEN_EOF     = 0,
};

int gcli_gitconf_parser_next_token(struct gcli_gitconf_parser *p);
int gcli_gitconf_parser_run(struct gcli_gitconf_parser *p, struct gcli_cmd_vcs_ctx *out);

#endif /* GCLI_CMD_VCS_GITCONFPARSER_H */
