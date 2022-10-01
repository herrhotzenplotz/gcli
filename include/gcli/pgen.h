#ifndef PGEN_H
#define PGEN_H

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

struct arrayparser {
    char *name;
    char *returntype;
    char *parser;
};

void yyerror(const char *message);

/* Functions to dump data before starting the actual parser */
void header_dump_c(void);
void header_dump_h(void);

/* Functions called while parsing */
void objparser_dump_c(struct objparser *);
void objparser_dump_h(struct objparser *);
void objparser_dump_plain(struct objparser *);

void arrayparser_dump_c(struct arrayparser *);
void arrayparser_dump_h(struct arrayparser *);

void include_dump_c(const char *);
void include_dump_h(const char *);

/* Functions called after parsing */
void footer_dump_h(void);

#endif /* PGEN_H */
