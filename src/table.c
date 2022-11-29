/*
 * Copyright 2022 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/table.h>

#include <gcli/color.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <sn/sn.h>

/* A row */
struct gcli_tblrow;

/* Internal state of a table printer. We return a handle to it in
 * gcli_table_init. */
struct gcli_tbl {
	gcli_tblcoldef const *cols; /* user provided column definitons */
	int *col_widths;            /* minimum width of the columns */
	size_t cols_size;           /* size of above arrays */

	struct gcli_tblrow *rows;   /* list of rows */
	size_t rows_size;           /* number of rows */
};

struct gcli_tblrow {
	struct {
		char *text;             /* the text in the cell */
		char const *colour;     /* colour (ansi escape sequence) if
		                         * explicit fixed colour was given */
	} *cells;
};

/* Push a row into the table state */
static int
table_pushrow(struct gcli_tbl *const table, struct gcli_tblrow row)
{
	table->rows = realloc(table->rows, sizeof(*table->rows) * (table->rows_size + 1));
	if (!table->rows)
		return -1;

	table->rows[table->rows_size++] = row;

	return 0;
}

/** Initialize the internal state structure of the table printer. */
gcli_tbl
gcli_tbl_begin(gcli_tblcoldef const *const cols, size_t const cols_size)
{
	struct gcli_tbl *tbl;

    /* Allocate the structure and fill in the handle */
	tbl = calloc(sizeof(*tbl), 1);
	if (!tbl)
		return NULL;

	/* Reserve memory for the column sizes */
	tbl->col_widths = calloc(sizeof(*tbl->col_widths), cols_size);
	if (!tbl->col_widths) {
		free(tbl);
		return NULL;
	}

	/* Store the list of columns */
	tbl->cols = cols;
	tbl->cols_size = cols_size;

	/* Check the headers */
	for (size_t i = 0; i < cols_size; ++i) {

		/* Compute the header's length and use these as initial
		 * values */
		tbl->col_widths[i] = strlen(cols[i].name);
	}

	return tbl;
}

static void
table_freerow(struct gcli_tblrow *row, size_t const cols)
{
	for (size_t i = 0; i < cols; ++i)
		free(row->cells[i].text);

	free(row->cells);
	row->cells = NULL;
}

static int
tablerow_add_cell(struct gcli_tbl *const table,
                  struct gcli_tblrow *const row,
                  size_t const col, va_list vp)
{
	int cell_size = 0;

	/* Extract the explicit colour code */
	if (table->cols[col].flags & GCLI_TBLCOL_COLOUREXPL) {
		int code = va_arg(vp, int);

		/* don't free that! it's allocated and free'ed inside color.c */
		row->cells[col].colour = gcli_setcolor(code);
	} else if (table->cols[col].flags & GCLI_TBLCOL_256COLOUR) {
		uint64_t hexcode = va_arg(vp, uint64_t);

		/* see comment above */
		row->cells[col].colour = gcli_setcolor256(hexcode);
	}


	/* Process the content */
	switch (table->cols[col].type) {
	case GCLI_TBLCOLTYPE_INT: {
		row->cells[col].text = sn_asprintf("%d", va_arg(vp, int));
		cell_size = strlen(row->cells[col].text);
	} break;
	case GCLI_TBLCOLTYPE_STRING: {
		char *it = va_arg(vp, char *);
		row->cells[col].text = strdup(it);
		cell_size = strlen(it);
	} break;
	case GCLI_TBLCOLTYPE_SV: {
		sn_sv src = va_arg(vp, sn_sv);
		row->cells[col].text = sn_sv_to_cstr(src);
		cell_size = src.length;
	} break;
	case GCLI_TBLCOLTYPE_DOUBLE: {
		row->cells[col].text = sn_asprintf("%lf", va_arg(vp, double));
		cell_size = strlen(row->cells[col].text);
	} break;
	case GCLI_TBLCOLTYPE_BOOL: {
		/* Do not use real _Bool type as it triggers a compiler bug in
		 * LLVM clang 13 */
		int val = va_arg(vp, int);
		if (val) {
			row->cells[col].text = strdup("yes");
			cell_size = 3;
		} else {
			row->cells[col].text = strdup("no");
			cell_size = 2;
		}
	} break;
	default:
		return -1;
	}

	/* Update the column width if needed */
	if (table->col_widths[col] < cell_size)
		table->col_widths[col] = cell_size;

	return 0;
}

int
gcli_tbl_add_row(gcli_tbl _table, ...)
{
	va_list vp;
	struct gcli_tblrow row = {0};
	struct gcli_tbl *table = (struct gcli_tbl *)(_table);

	/* reserve array of cells */
	row.cells = calloc(sizeof(*row.cells), table->cols_size);
	if (!row.cells)
		return -1;

	va_start(vp, _table);

	/* Step through all the columns and print the cells */
	for (size_t i = 0; i < table->cols_size; ++i) {
		if (tablerow_add_cell(table, &row, i, vp) < 0) {
			table_freerow(&row, table->cols_size);
			va_end(vp);
			return -1;
		}
	}

	va_end(vp);

	/* Push the row into the table */
	if (table_pushrow(table, row) < 0) {
		table_freerow(&row, table->cols_size);
		return -1;
	}

	return 0;
}

static void
pad(size_t const n)
{
	for (size_t p = 0; p < n; ++p)
		putchar(' ');
}

static void
dump_row(struct gcli_tbl const *const table, size_t const i)
{
	struct gcli_tblrow const *const row = &table->rows[i];

	for (size_t col = 0; col < table->cols_size; ++col) {
		/* If right justified and not last column, print padding */
		if ((table->cols[col].flags & GCLI_TBLCOL_JUSTIFYR) &&
		    (col + 1) < table->cols_size)
			pad(table->col_widths[col] - strlen(row->cells[col].text));

		/* State color */
		if (table->cols[col].flags & GCLI_TBLCOL_STATECOLOURED)
			printf("%s", gcli_state_color_str(row->cells[col].text));
		else if (table->cols[col].flags &
		         (GCLI_TBLCOL_COLOUREXPL|GCLI_TBLCOL_256COLOUR))
			printf("%s", row->cells[col].colour);

		/* Bold */
		if (table->cols[col].flags & GCLI_TBLCOL_BOLD)
			printf("%s", gcli_setbold());

		/* Print cell if it is not NULL, otherwise indicate it by
		 * printing <empty> */
		printf("%s  ", row->cells[col].text ? row->cells[col].text : "<empty>");

		/* End color */
		if (table->cols[col].flags &
		    (GCLI_TBLCOL_STATECOLOURED
		     |GCLI_TBLCOL_COLOUREXPL
		     |GCLI_TBLCOL_256COLOUR))
			printf("%s", gcli_resetcolor());

		/* Stop printing in bold */
		if (table->cols[col].flags & GCLI_TBLCOL_BOLD)
			printf("%s", gcli_resetbold());

		/* If left-justified and not last column, print padding */
		if (!(table->cols[col].flags & GCLI_TBLCOL_JUSTIFYR) &&
		    (col + 1) < table->cols_size)
			pad(table->col_widths[col] - strlen(row->cells[col].text));
	}
	putchar('\n');
}

static int
gcli_tbl_dump(gcli_tbl const _table)
{
	struct gcli_tbl const *const table = (struct gcli_tbl const *const)_table;

	for (size_t i = 0; i < table->cols_size; ++i) {
		printf("%s  ", table->cols[i].name);

		if ((i + 1) < table->cols_size)
			pad(table->col_widths[i] - strlen(table->cols[i].name));
	}
	printf("\n");

	for (size_t i = 0; i < table->rows_size; ++i) {
		dump_row(table, i);
	}

	return 0;
}

static void
gcli_tbl_free(gcli_tbl _table)
{
	struct gcli_tbl *tbl = (struct gcli_tbl *)_table;

	for (size_t row = 0; row < tbl->rows_size; ++row)
		table_freerow(&tbl->rows[row], tbl->cols_size);

	free(tbl->rows);
	free(tbl->col_widths);
	free(tbl);
}

void
gcli_tbl_end(gcli_tbl tbl)
{
	gcli_tbl_dump(tbl);
	gcli_tbl_free(tbl);
}
