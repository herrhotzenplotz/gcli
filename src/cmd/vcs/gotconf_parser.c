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

#include <gcli/cmd/vcs/gotconf_parser.h>
#include <gcli/port/string.h>
#include <gcli/url.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static char const *
token_names[] = {
	[GCLI_GOTCONF_TOKEN_EOF] = "end of line",
	[GCLI_GOTCONF_TOKEN_LITERAL] = "string literal",
	[GCLI_GOTCONF_TOKEN_OCURLY] = "{",
	[GCLI_GOTCONF_TOKEN_CCURLY] = "}",
};

enum {
	OK = 0,
	ERR = -1,
	SKIP = -2,
};

static int
lexerr(struct gcli_gotconf_parser *p, char const *const msg)
{
	p->error_message = strdup(msg);
	return -1;
}

static int
syntax(struct gcli_gotconf_parser *p, char const *const fmt, ...)
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

/* eat whitespace */
static void
ws(struct gcli_gotconf_parser *p)
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
lex_unquoted_literal(struct gcli_gotconf_parser *p)
{
	size_t len = 0;

	len = strcspn(p->head, " \n\t;,{}");

	if (p->token_text)
		free(p->token_text);

	p->token_text = gcli_strndup(p->head, len);
	p->head += len;

	return GCLI_GOTCONF_TOKEN_LITERAL;
}

static int
lex_quoted_literal(struct gcli_gotconf_parser *p)
{
	size_t len = 0;

	p->head += 1; /* skip '"' */

	len = strcspn(p->head, "\"");

	if (p->token_text)
		free(p->token_text);

	p->token_text = gcli_strndup(p->head, len);
	p->head += len + 1;

	return GCLI_GOTCONF_TOKEN_LITERAL;
}

int
gcli_gotconf_parser_next_token(struct gcli_gotconf_parser *p)
{
	if (p->head == NULL)
		return lexerr(p, "input buffer is null pointer");

	ws(p);

	switch (p->head[0]) {
	case '\0': return p->head[0];
	case '{':
	case '}': return *p->head++;
	case '"': return lex_quoted_literal(p);
	default: return lex_unquoted_literal(p);
	}
}

static int
expect_tokentext(struct gcli_gotconf_parser *p, char const *const text)
{
	if (strcmp(p->token_text, text))
		return syntax(p, "expected %s, got %s instead", text,
		              p->token_text);

	return 0;
}

static int
expect(struct gcli_gotconf_parser *p, int xtok)
{
	int ntok = gcli_gotconf_parser_next_token(p);

	if (ntok != xtok)
		return syntax(p, "expected %s, got %s instead",
		              token_names[xtok], token_names[ntok]);

	return 0;
}

static int
parse_list(struct gcli_gotconf_parser *p)
{
	int ntok;
	int depth = 1;

	while (depth) {
		ntok = gcli_gotconf_parser_next_token(p);
		if (ntok == GCLI_GOTCONF_TOKEN_EOF)
			return syntax(p, "unexpected end of file");

		if (ntok == GCLI_GOTCONF_TOKEN_OCURLY)
			depth += 1;

		if (ntok == GCLI_GOTCONF_TOKEN_CCURLY)
			depth -= 1;
	}

	return 0;
}

static int
parse_server(struct gcli_cmd_vcs_remote *remote, char const *server_url)
{
	struct gcli_url url = {0};
	int rc = 0;

	rc = gcli_parse_url(server_url, &url);
	if (rc < 0)
		return SKIP;

	remote->host = strdup(url.host);

	gcli_vcs_guess_forgetype_by_hostname(url.host, &remote->forge_type);
	gcli_url_free(&url);

	return OK;
}

static int
parse_path(struct gcli_cmd_vcs_remote *remote, char const *path)
{
	char *tmp;
	int n;

	/* skip over leading slash */
	if (path[0] == '/')
		path += 1;

	tmp = strrchr(path, '/');
	if (tmp == NULL)
		return SKIP;

	/* skip over leading slash */
	if (path[0] == '/')
		path += 1;

	remote->owner = gcli_strndup(path, tmp - path);

	/* skip over '/' */
	tmp += 1;

	n = strlen(tmp);
	if (n > 4 && strcmp(tmp + (n - 4), ".git") == 0)
		n -= 4;

	remote->repo = gcli_strndup(tmp, n);

	return OK;
}

static int
parse_remote(struct gcli_gotconf_parser *p,
             struct gcli_cmd_vcs_remote *remote)
{
	int final_rc = 0;
	int ntok = gcli_gotconf_parser_next_token(p);

	if (ntok != GCLI_GOTCONF_TOKEN_LITERAL)
		return syntax(p, "expected remote name literal, got %s instead",
		              token_names[ntok]);

	remote->name = strdup(p->token_text);
	expect(p, '{');

	/* iterate until we get the closing '}' */
	for (;;) {
		char *key, *value;
		int rc = 0;

		ntok = gcli_gotconf_parser_next_token(p);
		if (ntok == '}')
			break;

		if (ntok != GCLI_GOTCONF_TOKEN_LITERAL)
			return syntax(p, "expected a key literal, got %s instead",
			              token_names[ntok]);

		key = strdup(p->token_text);

		/* When the next token is '{' we get a list of things.
		 *
		 * We are not really interested in it, so just skip over it. */
		ntok = gcli_gotconf_parser_next_token(p);
		if (ntok == GCLI_GOTCONF_TOKEN_OCURLY) {
			free(key);

			rc = parse_list(p);
			if (rc < 0)
				return rc;

			continue;
		}

		/* otherwise continue */
		if (ntok != GCLI_GOTCONF_TOKEN_LITERAL) {
			syntax(p, "expected string literal or '{', got %s instead",
			       token_names[ntok]);

			free(key);

			return -1;
		}

		value = strdup(p->token_text);

		if (strcmp(key, "server") == 0)
			rc = parse_server(remote, value);
		else if (strcmp(key, "repository") == 0)
			rc = parse_path(remote, value);
		else
			rc = 0; /* ignore */

		free(value);
		free(key);

		if (rc == ERR)
			return rc;

		if (rc == SKIP)
			final_rc = SKIP;
	}

	return final_rc;
}

static int
parse_remotes(struct gcli_gotconf_parser *p,
              struct gcli_cmd_vcs_remotes *rs)
{
	for (;;) {
		struct gcli_cmd_vcs_remote *r;
		int next_token, rc;

		next_token = gcli_gotconf_parser_next_token(p);
		if (next_token == GCLI_GOTCONF_TOKEN_EOF)
			return 0;

		if (next_token != GCLI_GOTCONF_TOKEN_LITERAL)
			return syntax(p, "expected a literal 'remote'");

		if (expect_tokentext(p, "remote") < 0)
			return -1;

		r = calloc(1, sizeof(*r));

		rc = parse_remote(p, r);

		switch (rc) {
		case OK:
			TAILQ_INSERT_TAIL(rs, r, next);
			break;
		case ERR:
			return rc;
		case SKIP:
			free(r);
			break;
		}
	}
}

int
gcli_gotconf_parser_run(struct gcli_gotconf_parser *p,
                        struct gcli_cmd_vcs_remotes *out)
{
	p->line = 1;
	TAILQ_INIT(out);

	return parse_remotes(p, out);
}
