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

#ifndef CURL_H
#define CURL_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <sn/sn.h>

typedef struct gcli_fetch_buffer gcli_fetch_buffer;

struct gcli_fetch_buffer {
	char   *data;
	size_t  length;
};

int gcli_fetch(char const *url,
               char **pagination_next,
               gcli_fetch_buffer *out);

void gcli_curl(FILE *stream,
               char const *url,
               char const *content_type);

int gcli_fetch_with_method(char const *method,
                           char const *url,
                           char const *data,
                           char **pagination_next,
                           gcli_fetch_buffer *out);

void gcli_post_upload(char const *url,
                      char const *content_type,
                      void *buffer,
                      size_t buffer_size,
                      gcli_fetch_buffer *out);

void gcli_curl_gitea_upload_attachment(char const *url,
                                       char const *filename,
                                       gcli_fetch_buffer *out);

bool gcli_curl_test_success(char const *url);
char *gcli_urlencode(char const *);
sn_sv gcli_urlencode_sv(sn_sv const);
char *gcli_urldecode(char const *input);

#endif /* CURL_H */
