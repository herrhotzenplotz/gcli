/* This file is part of gcli.
 *
 * Copyright Nico Sonack <nsonack@herrhotzenplotz.de> */

#include <gcli/cmd/vcs/gitconf_parser.h>

#include <gcli/port/string.h>

#include <stdlib.h>

static int
lexerr(struct gcli_gitconf_parser *p, char const *const msg)
{
	p->error_message = strdup(msg);
	return -1;
}

/* eat whitespace */
static void
ws(struct gcli_gitconf_parser *p)
{
	for (;;) {
		char c = *p->head;

		if (c == '\n')
			p->line += 1;
		else if (c == ' ' || c == '\t' || c == '\r')
			/* just skip */;
		else
			break;

		p->head++;
	}
}

static int
lex_unquoted_literal(struct gcli_gitconf_parser *p)
{
	size_t len = 0;

	len = strcspn(p->head, " \n\t[]");

	if (p->token_text)
		free(p->token_text);

	p->token_text = gcli_strndup(p->head, len);
	p->head += len;

	return GCLI_GITCONF_TOKEN_LITERAL;
}

static int
lex_quoted_literal(struct gcli_gitconf_parser *p)
{
	size_t len = 0;

	p->head += 1; /* skip '"' */

	len = strcspn(p->head, "\"");

	if (p->token_text)
		free(p->token_text);

	p->token_text = gcli_strndup(p->head, len);
	p->head += len + 1;

	return GCLI_GITCONF_TOKEN_LITERAL;
}

int
gcli_gitconf_parser_next_token(struct gcli_gitconf_parser *p)
{
	if (p->head == NULL)
		return lexerr(p, "input buffer is null pointer");

	ws(p);

	switch (p->head[0]) {
	case '\0': return p->head[0];
	case '=':
	case '[':
	case ']': return *p->head++;
	case '"': return lex_quoted_literal(p);
	default: return lex_unquoted_literal(p);
	}
}

int
gcli_gitconf_parser_run(struct gcli_gitconf_parser *p,
                        struct gcli_cmd_vcs_ctx *out)
{
	(void) out;

	p->error_message = gcli_asprintf("%s: not yet implemented", __func__);
	return -1;
}
