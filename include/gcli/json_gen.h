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

#ifndef GCLI_JSON_GEN_H
#define GCLI_JSON_GEN_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdbool.h>
#include <stddef.h>

enum {
	GCLI_JSONGEN_ARRAY = 1,
	GCLI_JSONGEN_OBJECT = 2,
};

typedef struct gcli_jsongen gcli_jsongen;
struct gcli_jsongen {
	char *buffer;
	size_t buffer_size;
	size_t buffer_capacity;

	int scopes[32];           /* scope stack */
	size_t scopes_size;       /* scope stack pointer */

	bool await_object_value;  /* when in an object scope set to true if
	                           * we expect a value and not a key */
	bool first_elem;          /* first element in object/array */
};

int gcli_jsongen_init(gcli_jsongen *gen);
void gcli_jsongen_free(gcli_jsongen *gen);
char *gcli_jsongen_to_string(gcli_jsongen *gen);

int gcli_jsongen_begin_object(gcli_jsongen *gen);
int gcli_jsongen_end_object(gcli_jsongen *gen);
int gcli_jsongen_begin_array(gcli_jsongen *gen);
int gcli_jsongen_end_array(gcli_jsongen *gen);
int gcli_jsongen_objmember(gcli_jsongen *gen, char const *key);
int gcli_jsongen_number(gcli_jsongen *gen, long long num);
int gcli_jsongen_string(gcli_jsongen *gen, char const *value);

#endif /* GCLI_JSON_GEN_H */
