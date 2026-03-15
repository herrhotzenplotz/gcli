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

#ifndef PGEN_H
#define PGEN_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdbool.h>
#include <stdio.h>

/* PGen command line options */
enum { DUMP_PLAIN = 0, DUMP_C = 1, DUMP_H = 2 };
extern int   dumptype;
extern FILE *outfile;
extern char *outfilename;


/* Types used in the parser to represent nodes in the AST */
struct strlit { char *text; };
struct ident { char *text; };
struct objentry {
	enum { OBJENTRY_SIMPLE, OBJENTRY_ARRAY, OBJENTRY_CONTINUATION } kind; /* either a simple field or an array */
	char            *jsonname;
	char            *name;
	char            *type;
	char            *parser;
	struct objentry *next;      /* linked list */
};

struct objparser {
	enum { OBJPARSER_ENTRIES, OBJPARSER_SELECT } kind;
	char            *name;
	char            *returntype;
	bool            is_struct;
	struct objentry *entries;
	struct {
		char *fieldtype;
		char *fieldname;
	} select;
};

struct arrayparser {
	char *name;
	bool is_struct;
	char *returntype;
	char *parser;
};

void yyerror(char const *message);

/* Functions to dump data before starting the actual parser */
void header_dump_c(void);
void header_dump_h(void);

/* Functions called while parsing */
void objparser_dump_c(struct objparser *);
void objparser_dump_h(struct objparser *);
void objparser_dump_plain(struct objparser *);

void arrayparser_dump_c(struct arrayparser *);
void arrayparser_dump_h(struct arrayparser *);

void include_dump_c(char const *);
void include_dump_h(char const *);

/* Functions called after parsing */
void footer_dump_h(void);

#endif /* PGEN_H */
