/* This file is part of gcli
 *
 * See LICENSE included with this distribution.
 *
 * Copyright Nico Sonack <nsonack@herrhotzenplotz.de> */

#ifndef TESTS_UNIT_H
#define TESTS_UNIT_H

/* Simple single-header unit testing "framework". Emits TAP14. */

#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct unitctx {
	/* leave when all fails */
	jmp_buf main_ctx;
	int testno, subtestno;
	int hadfail;
};

#define UNIT_CTX_VAR _ctx
#define UNIT_CTX struct unitctx *UNIT_CTX_VAR
typedef void (*unitcase_fn)(UNIT_CTX);

#define TESTCASE(name) unit_run_test_case(UNIT_CTX_VAR, name, #name)
#define TESTSUITE static void unit_main(UNIT_CTX)

#define DEFINE_TESTCASE(name) static void name(UNIT_CTX)

/* very dumb but should suffice for most of what we do here */
static inline char *
unit_yamlescape(char const *in)
{
	size_t len = strlen(in) * 2 + 1; /* enough space */
	char *result, *hd, c;

	result = calloc(1, len);
	if (!result) {
		fprintf(stderr, "cannot allocate memory\n");
		abort();
	}

	hd = result;

	while ((c = *in++)) {
		if (c == '\n') {
			*hd++ = '\\';
			*hd++ = 'n';
		} else if (c == '"') {
			*hd++ = '\\';
			*hd++ = '"';
		} else {
			*hd++ = c;
		}
	}

	return result;
}

static inline void
unit_require(UNIT_CTX, char const *const file, int const line,
             char *expression, bool result)
{
	UNIT_CTX_VAR->subtestno += 1;
	if (!result) {
		UNIT_CTX_VAR->hadfail = 1;
		printf("    not ok %d - Requirement failed: %s\n"
		       "     ---\n"
		       "     at:\n"
		       "       file: %s\n"
		       "       line: %d\n"
		       "     ...\n"
		       "\n",
		       UNIT_CTX_VAR->subtestno, expression, file, line);

		longjmp(UNIT_CTX_VAR->main_ctx, 1);
	}

	printf("    ok %d - %s\n", UNIT_CTX_VAR->subtestno, expression);
}
#define REQUIRE(expr) unit_require(UNIT_CTX_VAR, __FILE__, __LINE__, #expr, expr)

static inline void
unit_require_streq(UNIT_CTX,
                   char const *const file,
                   int const line,
                   char const *const expression,
                   char const *actual,
                   char const *const expected)
{
	UNIT_CTX_VAR->subtestno += 1;

	if (strcmp(actual, expected)) {
		char *e_act, *e_exp;

		e_act = unit_yamlescape(actual);
		e_exp = unit_yamlescape(expected);

		UNIT_CTX_VAR->hadfail = 1;
		printf("    not ok %d - Requirement failed: %s\n"
		       "      ---\n"
		       "      wanted: \"%s\"\n"
		       "      found: \"%s\"\n"
		       "      at:\n"
		       "        file: %s\n"
		       "        line: %d\n"
		       "      ...\n"
		       "\n",
		       UNIT_CTX_VAR->subtestno, expression,
		       e_exp, e_act, file, line);

		free(e_act);
		free(e_exp);

		longjmp(UNIT_CTX_VAR->main_ctx, 1);
	}

	printf("    ok %d - %s\n", UNIT_CTX_VAR->subtestno, expression);
}
#define REQUIRE_STREQ(expr, expected) \
	unit_require_streq(UNIT_CTX_VAR, __FILE__, __LINE__, #expr, expr, expected)

static inline void
unit_check(UNIT_CTX, char const *const file, int const line,
           char *expression, bool result)
{
	UNIT_CTX_VAR->subtestno += 1;

	if (!result) {
		UNIT_CTX_VAR->hadfail = 1;
		printf("    not ok %d - %s\n"
		       "      ---\n"
		       "      at:\n"
		       "        file: %s\n"
		       "        line: %d\n"
		       "      ...\n",
		       UNIT_CTX_VAR->subtestno, expression, file, line);
	} else {
		printf("    ok %d - %s\n", UNIT_CTX_VAR->subtestno, expression);
	}
}
#define CHECK(expr) unit_check(UNIT_CTX_VAR, __FILE__, __LINE__, #expr, expr)
#define CHECK_EQ(expr, expected) \
	unit_check(UNIT_CTX_VAR, __FILE__, __LINE__, #expr " == " #expected, expr == expected)

static inline void
unit_check_streq(UNIT_CTX,
                 char const *const expression,
                 char const *actual,
                 char const *const expected,
                 char const *const file,
                 int const line)
{
	UNIT_CTX_VAR->subtestno += 1;

	if (actual == NULL || strcmp(actual, expected)) {
		char *e_act, *e_exp;

		e_act = unit_yamlescape(actual ? actual : "");
		e_exp = unit_yamlescape(expected);

		UNIT_CTX_VAR->hadfail = 1;
		printf("    not ok %d - %s == \"%s\"\n"
		       "      ---\n"
		       "      wanted: \"%s\"\n"
		       "      found: \"%s\"\n"
		       "      at:\n"
		       "        file: %s\n"
		       "        line: %d\n"
		       "      ...\n"
		       "\n",
		       UNIT_CTX_VAR->subtestno, expression, expected,
		       e_exp, e_act, file, line);

		free(e_act);
		free(e_exp);
	} else {
		printf("    ok %d - %s == \"%s\"\n", UNIT_CTX_VAR->subtestno, expression, expected);
	}
}
#define CHECK_STREQ(expr, expected) unit_check_streq(UNIT_CTX_VAR, #expr, expr, expected, __FILE__, __LINE__)

static void unit_main(struct unitctx *);

static inline void
unit_run_test_case(UNIT_CTX, unitcase_fn fn, char const *const case_name)
{
	int rc = 0;

	UNIT_CTX_VAR->testno += 1;
	UNIT_CTX_VAR->subtestno = 0;
	UNIT_CTX_VAR->hadfail = 0;

	printf("# Subtest: %s\n", case_name);

	rc = setjmp(UNIT_CTX_VAR->main_ctx);
	if (rc == 0) {
		fn(UNIT_CTX_VAR);
	}

	printf("    1..%d\n", UNIT_CTX_VAR->subtestno);

	if (UNIT_CTX_VAR->hadfail)
		printf("not ");

	printf("ok %d - %s\n", UNIT_CTX_VAR->testno, case_name);
}

int
main(void)
{
	struct unitctx ctx = {0};

	printf("TAP version 13\n");
	unit_main(&ctx);
	printf("1..%d\n", ctx.testno);

	return 0;
}

#endif /* TESTS_UNIT_H */
