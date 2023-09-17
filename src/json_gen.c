/*
 * Copyright 2023 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/json_gen.h>

#include <config.h>

#include <stdlib.h>
#include <string.h>

static void
grow_buffer(gcli_jsongen *gen)
{
	gen->buffer_capacity *= 2;
	gen->buffer = realloc(gen->buffer, gen->buffer_capacity);
}

int
gcli_jsongen_init(gcli_jsongen *gen)
{
	/* This will allocate a 32 byte buffer. We can optimise
	 * this for better allocation speed by analysing some statistics
	 * of how long usually the generated buffers are. */

	memset(gen, 0, sizeof(*gen));
	gen->buffer_capacity = 16;

	grow_buffer(gen);

	gen->first_elem = true;

	return 0;
}

void
gcli_jsongen_free(gcli_jsongen *gen)
{
	free(gen->buffer);
	gen->buffer = NULL;
	gen->buffer_size = 0;
	gen->buffer_capacity = 0;

	gen->scopes_size = 0;
}

char *
gcli_jsongen_to_string(gcli_jsongen *gen)
{
	char *buf = calloc(gen->buffer_size + 1, 1);

	return memcpy(buf, gen->buffer, gen->buffer_size);
}

static void
fit(gcli_jsongen *gen, size_t const n_chars)
{
	while (gen->buffer_capacity - gen->buffer_size < n_chars)
		grow_buffer(gen);
}

static int
push_scope(gcli_jsongen *gen, int const scope)
{
	if (gen->scopes_size >= (sizeof(gen->scopes) / sizeof(*gen->scopes)))
		return -1;

	gen->scopes[gen->scopes_size++] = scope;

	return 0;
}

static int
pop_scope(gcli_jsongen *gen)
{
	if (gen->scopes_size == 0)
		return -1;

	return gen->scopes[--gen->scopes_size];
}

static int
is_array_or_object_scope(gcli_jsongen *gen)
{
	return !!gen->scopes_size;
}

static void
append_str(gcli_jsongen *gen, char const *str)
{
	size_t const len = strlen(str);
	fit(gen, len);
	memcpy(gen->buffer + gen->buffer_size, str, len);
	gen->buffer_size += len;
}

static void
put_comma_if_needed(gcli_jsongen *gen)
{
	if (!gen->first_elem && is_array_or_object_scope(gen))
		append_str(gen, ", ");

	gen->first_elem = false;
}

static bool
is_object_scope(gcli_jsongen *gen)
{
	if (gen->scopes_size == 0)
		return false;

	return gen->scopes[gen->scopes_size - 1] == GCLI_JSONGEN_OBJECT;
}

int
gcli_jsongen_begin_object(gcli_jsongen *gen)
{
	/* Cannot put a json object into a json object key */
	if (is_object_scope(gen) && !gen->await_object_value)
		return -1;

	put_comma_if_needed(gen);

	if (push_scope(gen, GCLI_JSONGEN_OBJECT) < 0)
		return -1;

	append_str(gen, "{");

	gen->first_elem = true;

	return 0;
}

int
gcli_jsongen_end_object(gcli_jsongen *gen)
{
	if (pop_scope(gen) != GCLI_JSONGEN_OBJECT)
		return -1;

	append_str(gen, "}");

	gen->first_elem = false;

	return 0;
}

int
gcli_jsongen_begin_array(gcli_jsongen *gen)
{
	/* Cannot put a json array into a json object key */
	if (is_object_scope(gen) && !gen->await_object_value)
		return -1;

	put_comma_if_needed(gen);

	if (push_scope(gen, GCLI_JSONGEN_ARRAY) < 0)
		return -1;

	append_str(gen, "[");

	gen->first_elem = true;

	return 0;
}

int
gcli_jsongen_end_array(gcli_jsongen *gen)
{
	if (pop_scope(gen) != GCLI_JSONGEN_ARRAY)
		return -1;

	append_str(gen, "]");

	gen->first_elem = false;

	return 0;
}
