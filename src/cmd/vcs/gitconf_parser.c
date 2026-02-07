/* This file is part of gcli.
 *
 * Copyright Nico Sonack <nsonack@herrhotzenplotz.de> */

#include <gcli/cmd/vcs/gitconf_parser.h>

#include <gcli/port/string.h>

#include <stdio.h>
#include <stdlib.h>

enum {
	OK = 0,
	DONE = 1,
	SKIP = 2,
	ERR = -1,
};

static char const *
token_names[] = {
	[GCLI_GITCONF_TOKEN_EOF] = "end of line",
	[GCLI_GITCONF_TOKEN_LITERAL] = "string literal",
	[GCLI_GITCONF_TOKEN_OBRACK] = "[",
	[GCLI_GITCONF_TOKEN_CBRACK] = "]",
	[GCLI_GITCONF_TOKEN_EQUALS] = "=",
};

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

static int
syntax(struct gcli_gitconf_parser *p, char const *const fmt, ...)
{
	va_list vp;
	char *buf;
	size_t len;

	va_start(vp, fmt);
	len = vsnprintf(NULL, 0, fmt, vp);
	va_end(vp);

	buf = malloc(len + 1);

	va_start(vp, fmt);
	vsnprintf(buf, len + 1, fmt, vp);
	va_end(vp);

	p->error_message = gcli_asprintf("error on line %d: %s", p->line, buf);
	free(buf);

	return -1;
}

static int
expect(struct gcli_gitconf_parser *p, int xtok)
{
	int ntok = gcli_gitconf_parser_next_token(p);

	if (ntok != xtok)
		return syntax(p, "expected %s, got %s instead",
		              token_names[xtok], token_names[ntok]);

	return 0;
}

static int
parse_branch(struct gcli_gitconf_parser *p, struct gcli_cmd_vcs_ctx *vcsctx)
{
	int tok;
	struct gcli_cmd_vcs_branch *b;
	bool is_remote = false;

	b = calloc(1, sizeof(*b));

	/* "title" ']' (key = value)* */
	if (expect(p, GCLI_GITCONF_TOKEN_LITERAL) < 0)
		return -1;

	b->name = strdup(p->token_text);

	if (expect(p, ']') < 0)
		return -1;

	for (;;) {
		tok = gcli_gitconf_parser_next_token(p);
		switch (tok) {
		case GCLI_GITCONF_TOKEN_EOF:
		case '[':
			goto done;

		case GCLI_GITCONF_TOKEN_LITERAL:
			is_remote = !strcmp(p->token_text, "remote");
			if (expect(p, '=') < 0)
				return -1;

			if (expect(p, GCLI_GITCONF_TOKEN_LITERAL) < 0)
				return -1;

			if (is_remote)
				b->remote = strdup(p->token_text);

			break;

		default:
			return syntax(
				p,
				"expected beginning of section, end of "
				"file or key-value-pair, got %s instead",
				token_names[tok]
			);
		}
	}

done:
	TAILQ_INSERT_TAIL(&vcsctx->branches, b, next);
	return 0;
}

static int
parse_remote(struct gcli_gitconf_parser *p, struct gcli_cmd_vcs_ctx *vcsctx)
{
	(void) vcsctx;
	return syntax(p, "%s: not implemented");
}

/* The section parser expects the first token to point at the section type */
static int
skip_section(struct gcli_gitconf_parser *p)
{
	int tok = 0;

	for (;;) {
		tok = gcli_gitconf_parser_next_token(p);
		if (tok == GCLI_GITCONF_TOKEN_EOF)
			return DONE;

		if (tok == '[')
			return OK;
	}
}

int
gcli_gitconf_parser_run(struct gcli_gitconf_parser *p,
                        struct gcli_cmd_vcs_ctx *out)
{
	int tok, rc;

	TAILQ_INIT(&out->branches);
	TAILQ_INIT(&out->remotes);

	tok = gcli_gitconf_parser_next_token(p);
	if (tok == GCLI_GITCONF_TOKEN_EOF)
		return 0;

	if (tok != GCLI_GITCONF_TOKEN_OBRACK)
		return syntax(
			p,
			"expected %s at section beginning, got %s instead",
			token_names[GCLI_GITCONF_TOKEN_OBRACK],
			token_names[tok]
		);

	for (;;) {
		tok = gcli_gitconf_parser_next_token(p);
		if (tok == GCLI_GITCONF_TOKEN_EOF)
			return 0;

		if (tok != GCLI_GITCONF_TOKEN_LITERAL)
			return syntax(
				p, "expected section type, got %s instead",
				token_names[tok]
			);

		if (strcmp(p->token_text, "remote") == 0)
			rc = parse_remote(p, out);
		else if (strcmp(p->token_text, "branch") == 0)
			rc = parse_branch(p, out);
		else
			rc = skip_section(p);

		if (rc == DONE)
			return 0;

		if (rc < 0)
			return rc;
	}
}
