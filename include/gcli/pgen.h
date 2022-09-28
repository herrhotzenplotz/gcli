#ifndef PGEN_H
#define PGEN_H

/* Types used in the parser to represent nodes in the AST */
struct strlit { char *text; };
struct ident { char *text; };
struct objentry {
    enum { OBJENTRY_SIMPLE, OBJENTRY_ARRAY } kind; /* either a simple field or an array */
    char			*name;
    char			*type;
    char			*parser;
    struct objentry *next; /* linked list */
};

struct objparser {
    char            *name;
    char            *returntype;
    struct objentry *entries;
};

void yyerror(const char *message);

#endif /* PGEN_H */
