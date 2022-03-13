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

#include <ghcli/curl.h>
#include <ghcli/github/status.h>
#include <ghcli/github/config.h>
#include <ghcli/json_util.h>

#include <sn/sn.h>
#include <pdjson/pdjson.h>

static void
github_notification_parse_subject(
    struct json_stream *stream,
    ghcli_notification *it)
{
    if (json_next(stream) != JSON_OBJECT)
        errx(1, "Expected Notification Subject Object");

    enum json_type key_type;
    while ((key_type = json_next(stream)) == JSON_STRING) {
        size_t          len        = 0;
        const char     *key        = json_get_string(stream, &len);
        enum json_type  value_type = 0;

        if (strncmp("title", key, len) == 0)
            it->title = get_string(stream);
        else if (strncmp("type", key, len) == 0)
            it->type = get_string(stream);
        else {
            value_type = json_next(stream);

            switch (value_type) {
            case JSON_ARRAY:
                json_skip_until(stream, JSON_ARRAY_END);
                break;
            case JSON_OBJECT:
                json_skip_until(stream, JSON_OBJECT_END);
                break;
            default:
                break;
            }
        }
    }
}

static void
github_notification_parse_repository(
    struct json_stream *stream,
    ghcli_notification *it)
{
    if (json_next(stream) != JSON_OBJECT)
        errx(1, "Expected Notification Repository Object");

    enum json_type key_type;
    while ((key_type = json_next(stream)) == JSON_STRING) {
        size_t          len        = 0;
        const char     *key        = json_get_string(stream, &len);
        enum json_type  value_type = 0;

        if (strncmp("full_name", key, len) == 0)
            it->repository = get_string(stream);
        else {
            value_type = json_next(stream);

            switch (value_type) {
            case JSON_ARRAY:
                json_skip_until(stream, JSON_ARRAY_END);
                break;
            case JSON_OBJECT:
                json_skip_until(stream, JSON_OBJECT_END);
                break;
            default:
                break;
            }
        }
    }
}

static void
parse_github_notification(
    struct json_stream *stream,
    ghcli_notification *it)
{
    if (json_next(stream) != JSON_OBJECT)
        errx(1, "Expected Notification Object");

    enum json_type key_type;
    while ((key_type = json_next(stream)) == JSON_STRING) {
        size_t          len        = 0;
        const char     *key        = json_get_string(stream, &len);
        enum json_type  value_type = 0;

        if (strncmp("updated_at", key, len) == 0)
            it->date = get_string(stream);
        else if (strncmp("reason", key, len) == 0)
            it->reason = get_string(stream);
        else if (strncmp("subject", key, len) == 0)
            github_notification_parse_subject(stream, it);
        else if (strncmp("repository", key, len) == 0)
            github_notification_parse_repository(stream, it);
        else {
            value_type = json_next(stream);

            switch (value_type) {
            case JSON_ARRAY:
                json_skip_until(stream, JSON_ARRAY_END);
                break;
            case JSON_OBJECT:
                json_skip_until(stream, JSON_OBJECT_END);
                break;
            default:
                break;
            }
        }
    }
}

size_t
github_get_notifications(ghcli_notification **notifications, int count)
{
    char               *url                = NULL;
    ghcli_fetch_buffer  buffer             = {0};
    struct json_stream  stream             = {0};
    size_t              notifications_size = 0;

    #warning I was here

    url = sn_asprintf("%s/notifications", github_get_apibase());
    // TODO: Handle pagination
    ghcli_fetch(url, NULL, &buffer);

    json_open_buffer(&stream, buffer.data, buffer.length);
    json_set_streaming(&stream, 1);

    enum json_type next_token = json_next(&stream);

    while ((next_token = json_peek(&stream)) != JSON_ARRAY_END) {
        if (next_token != JSON_OBJECT)
            errx(1, "Unexpected non-object in notifications list");

        *notifications = realloc(
            *notifications,
            (notifications_size + 1) * sizeof(ghcli_notification));
        ghcli_notification *it = &(*notifications)[notifications_size];
        parse_github_notification(&stream, it);
        notifications_size += 1;
    }

    json_close(&stream);

    free(url);
    free(buffer.data);

    return notifications_size;
}
