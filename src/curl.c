/*
 * Copyright 2021 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <ctype.h>
#include <string.h>

#include <ghcli/config.h>
#include <ghcli/curl.h>
#include <ghcli/forges.h>
#include <ghcli/json_util.h>

#include <curl/curl.h>
#include <sn/sn.h>
#include <pdjson/pdjson.h>

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
             ghcli_forge()->get_api_error_string(result));
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

void
ghcli_fetch(
    const char *url,
    char **pagination_next,
    ghcli_fetch_buffer *out)
{
    ghcli_fetch_with_method("GET", url, NULL, pagination_next, out);
}

bool
ghcli_curl_test_success(const char *url)
{
    CURLcode           ret;
    CURL              *session;
    ghcli_fetch_buffer buffer = {0};
    long               status_code;
    bool               is_success = true;

    session = curl_easy_init();

    curl_easy_setopt(session, CURLOPT_URL, url);
    curl_easy_setopt(session, CURLOPT_BUFFERSIZE, 102400L);
    curl_easy_setopt(session, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(session, CURLOPT_MAXREDIRS, 50L);
    curl_easy_setopt(session, CURLOPT_FTP_SKIP_PASV_IP, 1L);
    curl_easy_setopt(session, CURLOPT_USERAGENT, "curl/7.78.0");
    curl_easy_setopt(
        session, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
    curl_easy_setopt(session, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(session, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(session, CURLOPT_WRITEFUNCTION, fetch_write_callback);
    curl_easy_setopt(session, CURLOPT_FAILONERROR, 0L);

    ret = curl_easy_perform(session);

    if (ret != CURLE_OK) {
        is_success = false;
    } else {
        curl_easy_getinfo(session, CURLINFO_RESPONSE_CODE, &status_code);

        if (status_code >= 300L)
            is_success = false;
    }

    free(buffer.data);
    curl_easy_cleanup(session);

    return is_success;
}

void
ghcli_curl(FILE *stream, const char *url, const char *content_type)
{
    CURLcode            ret;
    CURL               *session;
    struct curl_slist  *headers;
    ghcli_fetch_buffer  buffer      = {0};
    char               *auth_header = NULL;

    headers = NULL;

    if (content_type)
        headers = curl_slist_append(headers, content_type);

    auth_header = ghcli_config_get_authheader();
    headers     = curl_slist_append(headers, auth_header);

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

    free(auth_header);
}

static size_t
fetch_header_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    char **out = userdata;

    size_t sz          = size * nmemb;
    sn_sv  buffer      = sn_sv_from_parts(ptr, sz);
    sn_sv  header_name = sn_sv_chop_until(&buffer, ':');

    /* Despite what the documentation says, this header is called
     * "link" not "Link". Webdev ftw /sarc */
    if (sn_sv_eq_to(header_name, "link")) {
        buffer.data   += 1;
        buffer.length -= 1;
        buffer         = sn_sv_trim_front(buffer);
        *out           = sn_strndup(buffer.data, buffer.length);
    }

    return sz;
}

static char *
parse_link_header(char *_header)
{
    sn_sv header = SV(_header);
    sn_sv entry  = {0};

    /* Iterate through the comma-separated list of link relations */
    while ((entry = sn_sv_chop_until(&header, ',')).length > 0) {
        entry = sn_sv_trim(entry);

        /* the entries have semicolon-separated fields like so:
         * <url>; rel=\"next\"
         *
         * This chops off the url and then looks at the rest.
         *
         * We're making lots of assumptions about the input data here
         * without sanity checking it. If it fails, we will know. Most
         * likely a segfault. */
        sn_sv almost_url = sn_sv_chop_until(&entry, ';');

        if (sn_sv_eq_to(entry, "; rel=\"next\"")) {
            /* Skip the triangle brackets around the url */
            almost_url.data   += 1;
            almost_url.length -= 2;
            almost_url         = sn_sv_trim(almost_url);
            return sn_sv_to_cstr(almost_url);
        }

        /* skip the comma if we have enough data */
        if (header.length > 0) {
            header.length -= 1;
            header.data   += 1;
        }
    }

    return NULL;
}

void
ghcli_fetch_with_method(
    const char  *method,
    const char  *url,
    const char  *data,
    char       **pagination_next,
    ghcli_fetch_buffer *out)
{
    CURLcode           ret;
    CURL              *session;
    struct curl_slist *headers;
    char              *link_header = NULL;

    char *auth_header = ghcli_config_get_authheader();

    headers = NULL;
    headers = curl_slist_append(
        headers,
        "Accept: application/vnd.github.v3+json");
    headers = curl_slist_append(
        headers,
        "Content-Type: application/json");
    headers = curl_slist_append(headers, auth_header);

    *out = (ghcli_fetch_buffer) {0};

    session = curl_easy_init();

    curl_easy_setopt(session, CURLOPT_URL, url);

    if (data)
        curl_easy_setopt(session, CURLOPT_POSTFIELDS, data);

    curl_easy_setopt(session, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(session, CURLOPT_USERAGENT, "curl/7.79.1");
    curl_easy_setopt(session, CURLOPT_CUSTOMREQUEST, method);
    curl_easy_setopt(session, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(session, CURLOPT_WRITEDATA, out);
    curl_easy_setopt(session, CURLOPT_WRITEFUNCTION, fetch_write_callback);
    curl_easy_setopt(session, CURLOPT_FAILONERROR, 0L);
    curl_easy_setopt(session, CURLOPT_HEADERFUNCTION, fetch_header_callback);
    curl_easy_setopt(session, CURLOPT_HEADERDATA, &link_header);

    ret = curl_easy_perform(session);
    ghcli_curl_check_api_error(session, ret, url, out);

    if (link_header && pagination_next)
        *pagination_next = parse_link_header(link_header);

    free(link_header);

    curl_easy_cleanup(session);
    session = NULL;
    curl_slist_free_all(headers);
    headers = NULL;

    free(auth_header);
}

void
ghcli_post_upload(
    const char         *url,
    const char         *content_type,
    void               *buffer,
    size_t              buffer_size,
    ghcli_fetch_buffer *out)
{
    CURLcode              ret;
    CURL                 *session;
    struct curl_slist    *headers;

    char *auth_header = ghcli_config_get_authheader();
    char *contenttype_header = sn_asprintf(
        "Content-Type: %s",
        content_type);
    char *contentsize_header = sn_asprintf(
        "Content-Length: %zu",
        buffer_size);

    headers = NULL;
    headers = curl_slist_append(
        headers,
        "Accept: application/vnd.github.v3+json");
    headers = curl_slist_append(headers, auth_header);
    headers = curl_slist_append(headers, contenttype_header);
    headers = curl_slist_append(headers, contentsize_header);

    session = curl_easy_init();

    curl_easy_setopt(session, CURLOPT_URL, url);
    curl_easy_setopt(session, CURLOPT_POST, 1L);
    curl_easy_setopt(session, CURLOPT_POSTFIELDS, buffer);
    curl_easy_setopt(session, CURLOPT_POSTFIELDSIZE, (long)buffer_size);

    curl_easy_setopt(session, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(session, CURLOPT_USERAGENT, "curl/7.79.1");
    curl_easy_setopt(session, CURLOPT_WRITEDATA, out);
    curl_easy_setopt(session, CURLOPT_WRITEFUNCTION, fetch_write_callback);

    ret = curl_easy_perform(session);
    ghcli_curl_check_api_error(session, ret, url, out);

    curl_easy_cleanup(session);
    session = NULL;
    curl_slist_free_all(headers);
    headers = NULL;

    free(auth_header);
    free(contentsize_header);
    free(contenttype_header);
}

sn_sv
ghcli_urlencode_sv(sn_sv _input)
{
    size_t  input_len;
    size_t  output_len;
    size_t  i;
    char   *output;
    char   *input;

    input      = _input.data;
    input_len  = _input.length;
    output     = calloc(1, 3 * input_len + 1);
    output_len = 0;

    for (i = 0; i < input_len; ++i) {
        if (!isalnum(input[i]) && input[i] != '-') {
            unsigned val = (input[i] & 0xFF);
            snprintf(output + output_len, 4, "%%%02.2X", val);
            output_len += 3;
        } else {
            output[output_len++] = input[i];
        }
    }

    return sn_sv_from_parts(output, output_len);
}

char *
ghcli_urlencode(const char *input)
{
    sn_sv encoded = ghcli_urlencode_sv(SV((char *)input));
    return encoded.data;
}
