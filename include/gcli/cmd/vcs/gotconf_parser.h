/* This file is part of gcli.
 *
 * Copyright Nico Sonack <nsonack@herrhotzenplotz.de> */

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
