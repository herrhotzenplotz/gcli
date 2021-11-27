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
#include <ghcli/config.h>
#include <ghcli/json_util.h>

#include <curl/curl.h>
#include <sn/sn.h>
#include <pdjson/pdjson.h>

const char *
ghcli_api_error_string(ghcli_fetch_buffer *it)
{
    struct json_stream stream = {0};
    enum json_type     next   = JSON_NULL;

    if (!it->length)
        return NULL;

    json_open_buffer(&stream, it->data, it->length);
    json_set_streaming(&stream, true);

    while ((next = json_next(&stream)) != JSON_OBJECT_END) {
        char *key = get_string(&stream);
        if (strcmp(key, "message") == 0)
            return get_string(&stream);

        free(key);
    }

    return "<No message key in error response object>";
}

static void
ghcli_curl_check_api_error(
    CURL *curl,
    CURLcode code,
    const char *url,
    ghcli_fetch_buffer *result)
{
    long status_code = 0;

    if (code != CURLE_OK)
        errx(1,
             "error: request to %s failed\n"
             "     : curl error: %s",
             url,
             curl_easy_strerror(code));

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);

    if (status_code >= 300L) {
        errx(1,
             "error: request to %s failed with code %ld\n"
             "     : API error: %s",
             url, status_code,
             ghcli_api_error_string(result));
    }
}

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
    headers = curl_slist_append(
        headers,
        "Accept: application/vnd.github.v3.full+json");

    session = curl_easy_init();

    curl_easy_setopt(session, CURLOPT_URL, url);
    curl_easy_setopt(session, CURLOPT_BUFFERSIZE, 102400L);
    curl_easy_setopt(session, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(session, CURLOPT_MAXREDIRS, 50L);
    curl_easy_setopt(session, CURLOPT_FTP_SKIP_PASV_IP, 1L);
    curl_easy_setopt(session, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(session, CURLOPT_USERAGENT, "curl/7.78.0");
    curl_easy_setopt(
        session, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
    curl_easy_setopt(session, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(session, CURLOPT_WRITEDATA, out);
    curl_easy_setopt(session, CURLOPT_WRITEFUNCTION, fetch_write_callback);
    curl_easy_setopt(session, CURLOPT_FAILONERROR, 0L);

    ret = curl_easy_perform(session);
    ghcli_curl_check_api_error(session, ret, url, out);

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
    ghcli_fetch_buffer buffer = {0};

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
    curl_easy_setopt(
        session, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
    curl_easy_setopt(session, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(session, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(session, CURLOPT_WRITEFUNCTION, fetch_write_callback);
    curl_easy_setopt(session, CURLOPT_FAILONERROR, 0L);

    ret = curl_easy_perform(session);
    ghcli_curl_check_api_error(session, ret, url, &buffer);

    fwrite(buffer.data, 1, buffer.length, stream);
    free(buffer.data);

    curl_easy_cleanup(session);
    curl_slist_free_all(headers);
}

void
ghcli_fetch_with_method(
    const char *method,
    const char *url,
    const char *data,
    ghcli_fetch_buffer *out)
{
    CURLcode           ret;
    CURL              *session;
    struct curl_slist *headers;

    const char *auth_header = sn_asprintf(
        "Authorization: token "SV_FMT"",
        SV_ARGS(ghcli_config_get_token()));

    headers = NULL;
    headers = curl_slist_append(
        headers,
        "Accept: application/vnd.github.v3+json");
    headers = curl_slist_append(headers, auth_header);

    session = curl_easy_init();

    curl_easy_setopt(session, CURLOPT_URL, url);
    curl_easy_setopt(session, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(session, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(session, CURLOPT_USERAGENT, "curl/7.79.1");
    curl_easy_setopt(session, CURLOPT_CUSTOMREQUEST, method);
    curl_easy_setopt(session, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(session, CURLOPT_WRITEDATA, out);
    curl_easy_setopt(session, CURLOPT_WRITEFUNCTION, fetch_write_callback);
    curl_easy_setopt(session, CURLOPT_FAILONERROR, 0L);

    ret = curl_easy_perform(session);
    ghcli_curl_check_api_error(session, ret, url, out);

    curl_easy_cleanup(session);
    session = NULL;
    curl_slist_free_all(headers);
    headers = NULL;

    free((void *)auth_header);
}


void
ghcli_perform_submit_pr(ghcli_submit_pull_options opts, ghcli_fetch_buffer *out)
{
    /* TODO : JSON Injection */
    char *post_fields = sn_asprintf(
        "{\"head\":\""SV_FMT"\",\"base\":\""SV_FMT"\", "
        "\"title\": \""SV_FMT"\", \"body\": \""SV_FMT"\" }",
        SV_ARGS(opts.from),
        SV_ARGS(opts.to),
        SV_ARGS(opts.title),
        SV_ARGS(opts.body));
    char *url         = sn_asprintf(
        "https://api.github.com/repos/"SV_FMT"/pulls",
        SV_ARGS(opts.in));

    ghcli_fetch_with_method("POST", url, post_fields, out);
    free(post_fields);
    free(url);
}

void
ghcli_perform_submit_comment(
    ghcli_submit_comment_opts opts,
    ghcli_fetch_buffer *out)
{
    char *post_fields = sn_asprintf(
        "{ \"body\": \""SV_FMT"\" }",
        SV_ARGS(opts.message));
    char *url         = sn_asprintf(
        "https://api.github.com/repos/%s/%s/issues/%d/comments",
        opts.org, opts.repo, opts.issue);

    ghcli_fetch_with_method("POST", url, post_fields, out);
    free(post_fields);
    free(url);
}

void
ghcli_perform_submit_issue(
    ghcli_submit_issue_options opts,
    ghcli_fetch_buffer *out)
{
    char *post_fields = sn_asprintf(
        "{ \"title\": \""SV_FMT"\", \"body\": \""SV_FMT"\" }",
        SV_ARGS(opts.title), SV_ARGS(opts.body));
    char *url         = sn_asprintf(
        "https://api.github.com/repos/"SV_FMT"/issues",
        SV_ARGS(opts.in));

    ghcli_fetch_with_method("POST", url, post_fields, out);
    free(post_fields);
    free(url);
}
