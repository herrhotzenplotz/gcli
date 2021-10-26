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
fetch_write_callback(char *in, size_t size, size_t nmemb, void *data)
{
    ghcli_fetch_buffer *out = data;

    out->data = realloc(out->data, out->length + size * nmemb);
    memcpy(&(out->data[out->length]), in, size * nmemb);
    out->length += size * nmemb;

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
    headers = curl_slist_append(headers, "Accept: application/vnd.github.v3.full+json");

    session = curl_easy_init();

    curl_easy_setopt(session, CURLOPT_URL, url);
    curl_easy_setopt(session, CURLOPT_BUFFERSIZE, 102400L);
    curl_easy_setopt(session, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(session, CURLOPT_MAXREDIRS, 50L);
    curl_easy_setopt(session, CURLOPT_FTP_SKIP_PASV_IP, 1L);
    curl_easy_setopt(session, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(session, CURLOPT_USERAGENT, "curl/7.78.0");
    curl_easy_setopt(session, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
    curl_easy_setopt(session, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(session, CURLOPT_WRITEDATA, out);
    curl_easy_setopt(session, CURLOPT_WRITEFUNCTION, fetch_write_callback);

    ret = curl_easy_perform(session);
    if (ret != CURLE_OK)
        errx(1, "Unable to perform GET request to %s: %s", url, curl_easy_strerror(ret));

    curl_easy_cleanup(session);
    curl_slist_free_all(headers);

#if 0
    FILE *f = fopen("foo.dat", "w");
    fwrite(out->data, 1, out->length, f);
    fclose(f);
#endif

    return 0;
}

void
ghcli_curl(FILE *stream, const char *url, const char *content_type)
{
    CURLcode           ret;
    CURL              *session;
    struct curl_slist *headers;

    headers = NULL;
    headers = curl_slist_append(headers, content_type);

    session = curl_easy_init();

    curl_easy_setopt(session, CURLOPT_URL, url);
    curl_easy_setopt(session, CURLOPT_BUFFERSIZE, 102400L);
    curl_easy_setopt(session, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(session, CURLOPT_MAXREDIRS, 50L);
    curl_easy_setopt(session, CURLOPT_FTP_SKIP_PASV_IP, 1L);
    curl_easy_setopt(session, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(session, CURLOPT_USERAGENT, "curl/7.78.0");
    curl_easy_setopt(session, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
    curl_easy_setopt(session, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(session, CURLOPT_WRITEDATA, stream);

    ret = curl_easy_perform(session);
    if (ret != CURLE_OK)
        errx(1, "Unable to perform GET request to %s: %s", url, curl_easy_strerror(ret));

    curl_easy_cleanup(session);
    curl_slist_free_all(headers);
}

void
ghcli_perform_submit_pr(ghcli_submit_pull_options opts, ghcli_fetch_buffer *out)
{
    CURLcode ret;
    CURL *session;
    struct curl_slist *headers;

    /* TODO : JSON Injection */
    const char *post_fields = sn_asprintf("{\"head\":\""SV_FMT"\",\"base\":\""SV_FMT"\", \"title\": \""SV_FMT"\" }",
                                          SV_ARGS(opts.from), SV_ARGS(opts.to), SV_ARGS(opts.title));
    const char *url         = sn_asprintf("https://api.github.com/repos/"SV_FMT"/pulls", SV_ARGS(opts.in));
    const char *auth_header = sn_asprintf("Authorization: token "SV_FMT"", SV_ARGS(opts.token));

    headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/vnd.github.v3+json");
    headers = curl_slist_append(headers, auth_header);

    session = curl_easy_init();

    curl_easy_setopt(session, CURLOPT_URL, url);
    curl_easy_setopt(session, CURLOPT_POSTFIELDS, post_fields);
    curl_easy_setopt(session, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(session, CURLOPT_USERAGENT, "curl/7.79.1");
    curl_easy_setopt(session, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(session, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(session, CURLOPT_WRITEDATA, out);
    curl_easy_setopt(session, CURLOPT_WRITEFUNCTION, fetch_write_callback);

    ret = curl_easy_perform(session);

    if (ret != CURLE_OK)
        errx(1, "Error performing POST request to GitHub api: %s", curl_easy_strerror(ret));

    curl_easy_cleanup(session);
    session = NULL;
    curl_slist_free_all(headers);
    headers = NULL;
}
