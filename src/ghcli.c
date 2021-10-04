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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sn/sn.h>

#include <curl/curl.h>
#include <pdjson.h>

struct json_buffer {
    char   *data;
    size_t  length;
};

static size_t
write_callback(char *in, size_t size, size_t nmemb, void *data)
{
    struct json_buffer *out = data;

    out->data = realloc(out->data, out->length + size * nmemb);
    memcpy(&(out->data[out->length]), in, size * nmemb);
    out->length += size + nmemb - 1; // <---- why? wtf?

    return size * nmemb;
}

static void
parse_issue_entry(json_stream *input)
{
    if (json_next(input) != JSON_OBJECT)
        errx(1, "Expected Issue Object");

    while (json_next(input) == JSON_STRING) {
        size_t          len        = 0;
        const char     *key        = json_get_string(input, &len);
        enum json_type  value_type = 0;

        if (strncmp("title", key, len) == 0) {
            value_type = json_next(input);
            if (value_type != JSON_STRING)
                errx(1, "Title is not a string");

            const char *title = json_get_string(input, &len);
            printf("%.*s\n", (int)len, title);
        } else {
            value_type = json_next(input);

            switch (value_type) {
            case JSON_ARRAY:
                json_skip_until(input, JSON_ARRAY_END);
                break;
            case JSON_OBJECT:
                json_skip_until(input, JSON_OBJECT_END);
                break;
            default:
                break;
            }
        }
    }
}

int
main(int argc, char *argv[])
{
    CURLcode           ret;
    CURL              *hnd;
    struct curl_slist *slist1;

    struct json_buffer json_buffer = {0};
    json_stream        stream      = {0};

    slist1 = NULL;
    slist1 = curl_slist_append(slist1, "Accept: application/vnd.github.v3+json");

    hnd = curl_easy_init();
    curl_easy_setopt(hnd, CURLOPT_URL, "https://api.github.com/repos/kraxarn/spotify-qt/issues");
    curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, slist1);
    curl_easy_setopt(hnd, CURLOPT_USERAGENT, "urmomxd");
    curl_easy_setopt(hnd, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
    curl_easy_setopt(hnd, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(hnd, CURLOPT_WRITEDATA, &json_buffer);
    curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, write_callback);

    ret = curl_easy_perform(hnd);

    FILE *f = fopen("foo.dat", "w");
    fwrite(json_buffer.data, json_buffer.length, 1, f);
    fclose(f);

    json_open_buffer(&stream, json_buffer.data, json_buffer.length);
    json_set_streaming(&stream, true);

    enum json_type next_token = json_next(&stream);

    while ((next_token = json_peek(&stream)) != JSON_ARRAY_END) {

        switch (next_token) {
        case JSON_ERROR:
            errx(1, "Parser error: %s", json_get_error(&stream));
            break;
        case JSON_OBJECT:
            parse_issue_entry(&stream);
            break;
        default:
            errx(1, "Unexpected json type in response");
            break;
        }

    }

    curl_easy_cleanup(hnd);
    hnd = NULL;
    curl_slist_free_all(slist1);
    slist1 = NULL;

    return EXIT_SUCCESS;
}
