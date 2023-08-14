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

#ifndef LABELS_H
#define LABELS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <gcli/gcli.h>

typedef struct gcli_label gcli_label;
typedef struct gcli_label_list gcli_label_list;

struct gcli_label {
	long      id;
	char     *name;
	char     *description;
	uint32_t  colour;
};

struct gcli_label_list {
	gcli_label *labels;
	size_t labels_size;
};

int gcli_get_labels(gcli_ctx *ctx, char const *owner, char const *reponame,
                    int max, gcli_label_list *out);

void gcli_free_label(gcli_label *label);

void gcli_free_labels(gcli_label_list *labels);

void gcli_print_labels(gcli_ctx *ctx, gcli_label_list const *list, int max);

int gcli_create_label(gcli_ctx *ctx, char const *owner, char const *repo,
                      gcli_label *label);

int gcli_delete_label(gcli_ctx *ctx, char const *owner, char const *repo,
                      char const *label);

#endif /* LABELS_H */
