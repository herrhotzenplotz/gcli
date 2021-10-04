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

#include <string.h>

#include <ghcli/curl.h>

#include <curl/curl.h>
#include <sn/sn.h>

static size_t
write_callback(char *in, size_t size, size_t nmemb, void *data)
{
    ghcli_fetch_buffer *out = data;

    out->data = realloc(out->data, out->length + size * nmemb);
    memcpy(&(out->data[out->length]), in, size * nmemb);
    out->length += size + nmemb - 1; // <---- why? wtf?

    return size * nmemb;
}

int
ghcli_fetch(const char *url, ghcli_fetch_buffer *out)
{
    CURLcode           ret;
    CURL              *session;
    struct curl_slist *headers;

    if (!out)
        errx(1, "ghcli_fetch: out parameter is null");

    headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/vnd.github.v3+json");

    session = curl_easy_init();

    curl_easy_setopt(session, CURLOPT_URL, url);
    curl_easy_setopt(session, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(session, CURLOPT_USERAGENT, "urmomxd");
    curl_easy_setopt(session, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
    curl_easy_setopt(session, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(session, CURLOPT_WRITEDATA, out);
    curl_easy_setopt(session, CURLOPT_WRITEFUNCTION, write_callback);

    ret = curl_easy_perform(session);
    if (ret != CURLE_OK)
        errx(1, "Unable to perform GET request to %s: %s", url, curl_easy_strerror(ret));

    curl_easy_cleanup(session);
    curl_slist_free_all(headers);

    return 0;
}
