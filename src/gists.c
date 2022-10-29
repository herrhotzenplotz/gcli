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

#include <gcli/gists.h>
#include <gcli/config.h>
#include <gcli/color.h>
#include <gcli/curl.h>
#include <gcli/json_util.h>

#include <gcli/github/config.h>

#include <pdjson/pdjson.h>

#include <templates/github/gists.h>

/* /!\ Before changing this, see comment in gists.h /!\ */
void
parse_github_gist_files_idiot_hack(json_stream *stream, gcli_gist *gist)
{
    enum json_type next = JSON_NULL;

    gist->files = NULL;
    gist->files_size = 0;

    if ((next = json_next(stream)) != JSON_OBJECT)
        errx(1, "Expected Gist Files Object");

    while ((next = json_next(stream)) == JSON_STRING) {
        gist->files = realloc(gist->files, sizeof(*gist->files) * (gist->files_size + 1));
        gcli_gist_file *it = &gist->files[gist->files_size++];
        parse_github_gist_file(stream, it);
    }

    if (next != JSON_OBJECT_END)
        errx(1, "Unclosed Gist Files Object");
}

int
gcli_get_gists(const char *user, int max, gcli_gist **out)
{
    char               *url      = NULL;
    char               *next_url = NULL;
    gcli_fetch_buffer   buffer   = {0};
    struct json_stream  stream   = {0};
    enum   json_type    next     = JSON_NULL;
    size_t              size     = 0;

    if (user)
        url = sn_asprintf(
            "%s/users/%s/gists",
            github_get_apibase(),
            user);
    else
        url = sn_asprintf("%s/gists", github_get_apibase());

    do {
        gcli_fetch(url, &next_url, &buffer);

        json_open_buffer(&stream, buffer.data, buffer.length);

        parse_github_gists(&stream, out, &size);

        json_close(&stream);
        free(buffer.data);
        free(url);
    } while ((url = next_url) && (max == -1 || (int)size < max));

    free(next_url);

    return (int)size;
}

static const char *
human_readable_size(size_t s)
{
    if (s < 1024) {
        return sn_asprintf(
            "%zu B",
            s);
    }

    if (s < 1024 * 1024) {
        return sn_asprintf(
            "%zu KiB",
            s / 1024);
    }

    if (s < 1024 * 1024 * 1024) {
        return sn_asprintf(
            "%zu MiB",
            s / (1024 * 1024));
    }

    return sn_asprintf(
        "%zu GiB",
        s / (1024 * 1024 * 1024));
}

static inline const char *
language_fmt(const char *it)
{
    if (it)
        return it;
    else
        return "Unknown";
}

static void
print_gist_file(gcli_gist_file *file)
{
    printf("      • %-15.15s  %-8.8s  %-s\n",
           language_fmt(file->language.data),
           human_readable_size(file->size),
           file->filename.data);
}

static void
print_gist(enum gcli_output_flags const flags, const gcli_gist *const gist)
{
    if (flags & OUTPUT_LONG) {
        printf("   ID : %s"SV_FMT"%s\n"
               "OWNER : %s"SV_FMT"%s\n"
               "DESCR : "SV_FMT"\n"
               " DATE : "SV_FMT"\n"
               "  URL : "SV_FMT"\n"
               " PULL : "SV_FMT"\n",
               gcli_setcolor(GCLI_COLOR_YELLOW), SV_ARGS(gist->id), gcli_resetcolor(),
               gcli_setbold(), SV_ARGS(gist->owner), gcli_resetbold(),
               SV_ARGS(gist->description),
               SV_ARGS(gist->date),
               SV_ARGS(gist->url),
               SV_ARGS(gist->git_pull_url));
        printf("FILES : %-15.15s  %-8.8s  %-s\n",
               "LANGUAGE", "SIZE", "FILENAME");

        for (size_t i = 0; i < gist->files_size; ++i)
            print_gist_file(&gist->files[i]);

        printf("\n");
    } else {
        printf("%32.*s  %s%-20.*s%s  %-20.*s  %-5zu  "SV_FMT"\n",
               SV_ARGS(gist->id),
               gcli_setbold(), SV_ARGS(gist->owner), gcli_resetbold(),
               SV_ARGS(gist->date),
               gist->files_size,
               SV_ARGS(gist->description));
    }
}

void
gcli_print_gists(
    enum gcli_output_flags  flags,
    gcli_gist              *gists,
    int                     gists_size)
{
    if (gists_size == 0) {
        puts("No Gists");
        return;
    }

    if (!(flags & OUTPUT_LONG)) {
        printf("%-32.32s  %-20.20s  %-20.20s  %-5.5s  %s\n",
               "ID", "OWNER", "DATE", "FILES", "DESCRIPTION");
    }

    /* output in reverse order if the sorted flag was enabled */
    if (flags & OUTPUT_SORTED) {
        for (int i = gists_size; i > 0; --i)
            print_gist(flags, &gists[i - 1]);
    } else {
        for (int i = 0; i < gists_size; ++i)
            print_gist(flags, &gists[i]);
    }
}

gcli_gist *
gcli_get_gist(const char *gist_id)
{
    char               *url    = NULL;
    gcli_fetch_buffer   buffer = {0};
    struct json_stream  stream = {0};
    gcli_gist          *it     = NULL;

    url = sn_asprintf("%s/gists/%s", github_get_apibase(), gist_id);

    gcli_fetch(url, NULL, &buffer);

    json_open_buffer(&stream, buffer.data, buffer.length);
    json_set_streaming(&stream, 1);

    it = calloc(sizeof(gcli_gist), 1);
    parse_github_gist(&stream, it);

    json_close(&stream);
    free(buffer.data);
    free(url);

    return it;
}

#define READ_SZ 4096
static size_t
read_file(FILE *f, char **out)
{
    size_t size = 0;

    *out = NULL;

    while (!feof(f) && !ferror(f)) {
        *out = realloc(*out, size + READ_SZ);
        size_t bytes_read = fread(*out + size, 1, READ_SZ, f);
        if (bytes_read == 0)
            break;
        size += bytes_read;
    }

    return size;
}

void
gcli_create_gist(gcli_new_gist opts)
{
    char              *url          = NULL;
    char              *post_data    = NULL;
    gcli_fetch_buffer  fetch_buffer = {0};
    sn_sv              read_buffer  = {0};
    sn_sv              content      = {0};

    read_buffer.length = read_file(opts.file, &read_buffer.data);
    content = gcli_json_escape(read_buffer);

    /* This API is documented very badly. In fact, I dug up how you're
     * supposed to do this from
     * https://github.com/phadej/github/blob/master/src/GitHub/Data/Gists.hs
     *
     * From this we can infer that we're supposed to create a JSON
     * object like so:
     *
     * {
     *  "description": "foobar",
     *  "public": true,
     *  "files": {
     *   "barf.exe": {
     *       "content": "#!/bin/sh\necho This file cannot be run in DOS mode"
     *   }
     *  }
     * }
     */

    /* TODO: Escape gist_description and file_name */
    url = sn_asprintf("%s/gists", github_get_apibase());
    post_data = sn_asprintf(
        "{\"description\":\"%s\",\"public\":true,\"files\":"
        "{\"%s\": {\"content\":\""SV_FMT"\"}}}",
        opts.gist_description,
        opts.file_name,
        SV_ARGS(content));

    gcli_fetch_with_method("POST", url, post_data, NULL, &fetch_buffer);
    gcli_print_html_url(fetch_buffer);

    free(read_buffer.data);
    free(fetch_buffer.data);
    free(url);
    free(post_data);
}

void
gcli_delete_gist(const char *gist_id, bool always_yes)
{
    char              *url    = NULL;
    gcli_fetch_buffer  buffer = {0};

    url = sn_asprintf(
        "%s/gists/%s",
        github_get_apibase(),
        gist_id);

    if (!always_yes && !sn_yesno("Are you sure you want to delete this gist?"))
        errx(1, "Aborted by user");

    gcli_fetch_with_method("DELETE", url, NULL, NULL, &buffer);

    free(buffer.data);
    free(url);
}
