/*
 * Copyright 2021, 2022 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#ifndef SNIPPETS_H
#define SNIPPETS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gcli/gcli.h>

typedef struct gcli_snippet gcli_snippet;
typedef struct gcli_snippet_list gcli_snippet_list;

struct gcli_snippet {
	int   id;
	char *title;
	char *filename;
	char *date;
	char *author;
	char *visibility;
	char *raw_url;
};

struct gcli_snippet_list {
	gcli_snippet *snippets;
	size_t snippets_size;
};

void gcli_snippets_free(gcli_snippet_list *const list);

int gcli_snippets_get(int const max, gcli_snippet_list *const out);

void gcli_snippets_print(enum gcli_output_flags const flags,
                         gcli_snippet_list const *const list,
                         int const max);

void gcli_snippet_delete(char const *snippet_id);

void gcli_snippet_get(char const *snippet_id);

#endif /* SNIPPETS_H */
