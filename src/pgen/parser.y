%{
#include <stdio.h>

#include <gcli/pgen.h>

FILE	*outfile	 = NULL;
char	*outfilename = NULL;
int		 dumptype	 = 0;

static void objparser_dump(struct objparser *);
static void include_dump(const char *);
static void header_dump(void);
static void footer_dump(void);

%}

%token PARSER IS OBJECT WITH AS USE FATARROW INCLUDE
%token OPAREN CPAREN SEMICOLON ARRAY OF COMMA SELECT

%union {
	struct strlit		strlit;
	struct ident		ident;
	struct objentry		objentry;
	struct objentry     *objentries;
	struct objparser    objparser;
}

%token	<strlit>		STRLIT
%token	<ident>			IDENT

%type	<objentry>		obj_entry
%type	<objentries>	obj_entries
%type	<objparser>		objparser

%%
input:			instruction input
		|
		;

instruction:	objparser SEMICOLON
				{
					objparser_dump(&($1));
				}
		|		INCLUDE STRLIT SEMICOLON
				{
					include_dump($2.text);
				}
		;

objparser:		PARSER IDENT IS OBJECT OF IDENT WITH OPAREN obj_entries CPAREN
				{
					$$.kind = OBJPARSER_ENTRIES;
					$$.name		  = $2.text;
					$$.returntype = $6.text;
					$$.entries	  = $9;
				}
		|		PARSER IDENT IS OBJECT OF IDENT SELECT STRLIT AS IDENT
				{
					$$.kind = OBJPARSER_SELECT;
					$$.name = $2.text;
					$$.returntype = $6.text;
					$$.select.fieldname = $8.text;
					$$.select.fieldtype = $10.text;
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
					$$.jsonname = $1.text;
					$$.kind		= OBJENTRY_SIMPLE;
					$$.name		= $3.text;
					$$.type		= $5.text;
					$$.parser	= NULL;
				}
		|		STRLIT FATARROW IDENT AS IDENT USE IDENT
				{
					$$.jsonname = $1.text;
					$$.kind		= OBJENTRY_SIMPLE;
					$$.name		= $3.text;
					$$.type		= $5.text;
					$$.parser	= $7.text;
				}
		|		STRLIT FATARROW IDENT AS ARRAY OF IDENT USE IDENT
				{
					$$.jsonname = $1.text;
					$$.kind		= OBJENTRY_ARRAY;
					$$.name		= $3.text;
					$$.type		= $7.text;
					$$.parser	= $9.text;
				}
		;
%%

#include <assert.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern FILE *yyin;
extern char *yyfile;

/******************************************************************************/

/* Table of functions to call when dumping various parts of the output
 * file */
struct {
	void (*dump_header)(void);
	void (*dump_footer)(void);
	void (*dump_objparser)(struct objparser *);
	void (*dump_include)(const char *);
} dumpers[] = {
	[DUMP_PLAIN] = {
		.dump_objparser = objparser_dump_plain
	},
	[DUMP_C] = {
		.dump_header	= header_dump_c,
		.dump_objparser = objparser_dump_c,
		.dump_include	= include_dump_c,
	},
	[DUMP_H] = {
		.dump_header = header_dump_h,
		.dump_objparser = objparser_dump_h,
		.dump_include = include_dump_h,
		.dump_footer = footer_dump_h,
	}
};

/* Helpers */
static void
objparser_dump(struct objparser *p)
{
	if (dumpers[dumptype].dump_objparser)
		dumpers[dumptype].dump_objparser(p);
	else
		yyerror("internal error: don't know how to dump an object parser");
}

static void
include_dump(const char *file)
{
	if (dumpers[dumptype].dump_include)
		dumpers[dumptype].dump_include(file);
}

static void
header_dump(void)
{
	if (dumpers[dumptype].dump_header)
		dumpers[dumptype].dump_header();
}

static void
footer_dump(void)
{
	if (dumpers[dumptype].dump_footer)
		dumpers[dumptype].dump_footer();
}

/******************************************************************************/
static void
usage(void)
{
	fprintf(stderr, "usage: pgen [-v] [-o outputfile] [-t c|plain] [...]\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "   -v       Print version and exit\n");
	fprintf(stderr, "   -o file  Dump output into the given file\n");
	fprintf(stderr, "   -t type  Type of the output. Can be either C or plain.\n");
}

int
main(int argc, char *argv[])
{
	int ch;

	while ((ch = getopt(argc, argv, "hvo:t:")) != -1) {
		switch (ch) {
		case 'o': {
			if (outfile)
				errx(1, "cannot specify -o more than once");
			outfile = fopen(optarg, "w");
			outfilename = optarg;
		} break;
		case 'v': {
			fprintf(stderr, "pgen version 0.1\n");
			exit(0);
		} break;
		case 't': {
			if (strcmp(optarg, "plain") == 0)
				dumptype = DUMP_PLAIN;
			else if (strcmp(optarg, "c") == 0)
				dumptype = DUMP_C;
			else if (strcmp(optarg, "h") == 0)
				dumptype = DUMP_H;
			else
				errx(1, "invalid dump type %s", optarg);
		} break;
		case '?':
		case '-':
		default:
			usage();
			exit(1);
		}
	}

	argc -= optind;
	argv += optind;

	if (!outfile) {
		outfile = stdout;
		outfilename = "<stdout>";
	}

	header_dump();

	if (argc) {
		for (int i = 0; i < argc; ++i) {
			yyfile = argv[i];
			yyin = fopen(argv[i], "r");
			yyparse();
			fclose(yyin);
		}
	} else  {
		yyfile = "<stdin>";
		yyin = stdin;
		yyparse();
	}

	footer_dump();

	fclose(outfile);

	return 0;
}
