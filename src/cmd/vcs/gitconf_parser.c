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

#include <gcli/cmd/vcs/gitconf_parser.h>

#include <gcli/port/string.h>
#include <gcli/url.h>

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
parse_remote_url(struct gcli_cmd_vcs_remote *const remote, char const *url_text)
{
	char *tmp;
	int rc = 0;
	size_t n = 0;
	struct gcli_url url = {0};

	rc = gcli_parse_url(url_text, &url);
	if (rc < 0) {
		fprintf(stderr, "gcli: failed to parse remote url: %s. "
		        "This is probably a bug.\n", url_text);

		goto bail;
	}

	/* probably a local clone */
	if (!url.host) {
		rc = 0;
		goto bail;
	}

	/* save away the host */
	remote->host = strdup(url.host);

	/* automagic forge type */
	gcli_vcs_guess_forgetype_by_hostname(url.host, &remote->forge_type);

	/* split owner/repo */
	tmp = strrchr(url.path, '/');
	if (tmp == NULL) {
		rc = -1;
		goto bail;
	}

	remote->owner = gcli_strndup(url.path, tmp - url.path);

	/* skip over '/' */
	tmp += 1;

	n = strlen(tmp);
	if (n > 4 && strcmp(tmp + (n - 4), ".git") == 0)
		n -= 4;

	remote->repo = gcli_strndup(tmp, n);
	rc = 0;

bail:
	gcli_url_free(&url);
	return rc;
}

static int
parse_remote(struct gcli_gitconf_parser *p, struct gcli_cmd_vcs_ctx *vcsctx)
{
	int tok;
	struct gcli_cmd_vcs_remote *r;
	bool is_url = false;

	r = calloc(1, sizeof(*r));

	/* "title" ']' (key = value)* */
	if (expect(p, GCLI_GITCONF_TOKEN_LITERAL) < 0)
		return -1;

	r->name = strdup(p->token_text);

	if (expect(p, ']') < 0)
		return -1;

	for (;;) {
		tok = gcli_gitconf_parser_next_token(p);
		switch (tok) {
		case GCLI_GITCONF_TOKEN_EOF:
		case '[':
			goto done;

		case GCLI_GITCONF_TOKEN_LITERAL:
			is_url = !strcmp(p->token_text, "url");
			if (expect(p, '=') < 0)
				return -1;

			if (expect(p, GCLI_GITCONF_TOKEN_LITERAL) < 0)
				return -1;

			if (is_url)
				parse_remote_url(r, p->token_text);

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
	TAILQ_INSERT_TAIL(&vcsctx->remotes, r, next);
	return 0;
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
