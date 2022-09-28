%{
#include <stdio.h>

extern int yyerror(const char *message);

#include <gcli/pgen.h>

%}

%token PARSER IS OBJECT WITH AS USE FATARROW
%token OPAREN CPAREN SEMICOLON ARRAY OF COMMA

%union {
	struct strlit		strlit;
	struct ident		ident;
	struct objentry		objentry;
	struct objentry     *objentries;
}

%token	<strlit>		STRLIT
%token	<ident>			IDENT

%type	<objentry>		obj_entry
%type	<objentries>	obj_entries

%%
input:			parser
		;
parser:			PARSER IDENT IS OBJECT OF IDENT WITH OPAREN obj_entries CPAREN SEMICOLON
				{
					printf("object parser: name = %s, type = %s\n", $2.text, $6.text);
					for (struct objentry *it = $9; it != NULL; it = it->next)
						printf("  entry: kind = %s, name = %s, type = %s, parser = %s\n",
							   it->kind == OBJENTRY_SIMPLE ? "simple" : "array",
							   it->name, it->type, it->parser);
				}
		;

obj_entries:	obj_entries COMMA obj_entry
				{
					$$ = malloc(sizeof(*($$)));
					*($$) = $3;
					$$->next = $1;
				}
		|		obj_entry
				{
					$$ = malloc(sizeof(*($$)));
					*($$) = $1;
					$$->next = NULL;
				}
		;

obj_entry:		STRLIT FATARROW IDENT AS IDENT
				{
					$$.kind	  = OBJENTRY_SIMPLE;
					$$.name	  = $3.text;
					$$.type	  = $5.text;
					$$.parser = NULL;
				}
		|		STRLIT FATARROW IDENT AS IDENT USE IDENT
				{
					$$.kind	  = OBJENTRY_SIMPLE;
					$$.name	  = $3.text;
					$$.type	  = $5.text;
					$$.parser = $7.text;
				}
		|		STRLIT FATARROW IDENT AS ARRAY OF IDENT USE IDENT
				{
					$$.kind	  = OBJENTRY_ARRAY;
					$$.name	  = $3.text;
					$$.type	  = $7.text;
					$$.parser = $9.text;
				}
		;
%%

extern FILE *yyin;
extern char *yyfile;

int
main(void)
{
	yyfile = "<stdin>";
	yyin = stdin;
	yyparse();
	return 0;
}
