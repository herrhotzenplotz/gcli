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

#include <ghcli/config.h>

#include <stdlib.h>
#include <unistd.h>

static struct ghcli_config {
    sn_sv api_token;
    sn_sv editor;

    sn_sv buffer;
    void *mmap_pointer;
} config;

void
ghcli_config_init(const char *file_path)
{
    if (!file_path) {
        file_path = getenv("XDG_CONFIG_PATH");
        if (!file_path) {
            file_path = getenv("HOME");
            if (!file_path)
                errx(1, "Neither XDG_CONFIG_PATH HOME nor set in env");

            file_path = sn_asprintf("%s/.config/", file_path);
        }

        file_path = sn_asprintf("%s/ghcli/config", file_path);
    }

    if (access(file_path, R_OK) < 0)
        err(1, "Cannot access config file at %s", file_path);

    int len = sn_mmap_file(file_path, &config.mmap_pointer);
    if (len < 0)
        err(1, "Unable to open config file");

    config.buffer = sn_sv_from_parts(config.mmap_pointer, len);
    config.buffer = sn_sv_trim_front(config.buffer);

    int curr_line = 1;
    while (config.buffer.length > 0) {
        sn_sv line = sn_sv_chop_until(&config.buffer, '\n');

        sn_sv key  = sn_sv_chop_until(&line, '=');

        key = sn_sv_trim(key);

        if (line.length == 0)
            errx(1, "%s:%d: Unexpected end of line",
                 file_path, curr_line);

        // Comments
        if (key.data[0] == '#') {
            curr_line++;
            continue;
        }

        line.data   += 1;
        line.length -= 1;

        sn_sv value = sn_sv_trim(line);

        if (sn_sv_eq_to(key, "api_token"))
            config.api_token = value;
        else if (sn_sv_eq_to(key, "editor"))
            config.editor = value;
        else
            errx(1, "%s:%d: unknown config entry '"SV_FMT"'",
                 file_path, curr_line, SV_ARGS(key));

        config.buffer = sn_sv_trim_front(config.buffer);
        curr_line++;
    }
}

const char *
ghcli_config_get_editor(void)
{
    if (config.editor.length)
        return sn_sv_to_cstr(config.editor);
    else
        return NULL;
}

sn_sv
ghcli_config_get_token(void)
{
    return config.api_token;
}
