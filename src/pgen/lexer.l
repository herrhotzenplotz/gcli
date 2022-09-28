%option noyywrap

%{
#include <stdio.h>
#include <stdlib.h>

#include <gcli/pgen.h>
#include "y.tab.h"

int yycol = 1, yyrow = 1;
char *yyfile = NULL;

void
yyerror(const char *msg)
{
	fprintf(stderr, "%s:%d:%d: error: %s\n", yyfile, yyrow, yycol, msg);
	exit(1);
}

%}

%%
\n                                 { yycol = 1; yyrow += 1;             }
[ \t]                              { yycol += 1;                        }
parser                             { yycol += yyleng; return PARSER;    }
is                                 { yycol += yyleng; return IS;        }
object                             { yycol += yyleng; return OBJECT;    }
with                               { yycol += yyleng; return WITH;      }
as                                 { yycol += yyleng; return AS;        }
use                                { yycol += yyleng; return USE;       }
array                              { yycol += yyleng; return ARRAY;     }
of                                 { yycol += yyleng; return OF;        }
=>                                 { yycol += yyleng; return FATARROW;  }
"("                                { yycol += yyleng; return OPAREN;    }
")"                                { yycol += yyleng; return CPAREN;    }
";"                                { yycol += yyleng; return SEMICOLON; }
","                                { yycol += yyleng; return COMMA;     }
[A-Za-z][A-Za-z0-9_]*              {
									   yycol += yyleng;
									   yylval.ident.text = strdup(yytext);
									   return IDENT;
								   }
\"[^\"]*\"                         {
									   yycol += yyleng;
									   yylval.strlit.text = strdup(yytext + 1);
									   yylval.strlit.text[strlen(yytext + 1) - 1] = '\0';
									   return STRLIT;
								   }
.                                  { yyerror("unrecognized character"); }
%%
