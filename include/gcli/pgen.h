#ifndef PGEN_H
#define PGEN_H

#include <stdio.h>

/* PGen command line options */
enum { DUMP_PLAIN = 0, DUMP_C = 1 };
extern int   dumptype;
extern FILE *outfile;
extern char *outfilename;


/* Types used in the parser to represent nodes in the AST */
struct strlit { char *text; };
struct ident { char *text; };
struct objentry {
    enum { OBJENTRY_SIMPLE, OBJENTRY_ARRAY } kind; /* either a simple field or an array */
    char            *jsonname;
    char			*name;
    char			*type;
    char			*parser;
    struct objentry *next; /* linked list */
};

struct objparser {
    enum { OBJPARSER_ENTRIES, OBJPARSER_SELECT } kind;
    char            *name;
    char            *returntype;
    struct objentry *entries;
    struct {
        char *fieldtype;
        char *fieldname;
    } select;
};

void yyerror(const char *message);

/* Functions to dump data before starting the actual parser */
void header_dump_c(void);

/* Functions called while parsing */
void objparser_dump_c(struct objparser *);
void objparser_dump_plain(struct objparser *);

void include_dump_c(const char *);

/* Functions called after parsing */
/* <none yet> */

#endif /* PGEN_H */
