/*
 * Copyright 2021 Nico Sonack <nsonack@outlook.com>
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

#include <stdio.h>
#include <stdlib.h>

#include <ghcli/issues.h>
#include <ghcli/pulls.h>
#include <ghcli/comments.h>

typedef struct ghcli_fetch_buffer ghcli_fetch_buffer;

struct ghcli_fetch_buffer {
    char   *data;
    size_t  length;
};

int  ghcli_fetch(const char *url, ghcli_fetch_buffer *out);
void ghcli_curl(FILE *stream, const char *url, const char *content_type);
void ghcli_fetch_with_method(const char *method, const char *url, const char *data, ghcli_fetch_buffer *out);
void ghcli_perform_submit_pr(ghcli_submit_pull_options opts, ghcli_fetch_buffer *out);
void ghcli_perform_submit_issue(ghcli_submit_issue_options opts, ghcli_fetch_buffer *out);
void ghcli_perform_submit_comment(ghcli_submit_comment_opts opts, ghcli_fetch_buffer *out);


#endif /* CURL_H */
