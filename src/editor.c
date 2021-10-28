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

#include <ghcli/editor.h>
#include <ghcli/config.h>

#include <sn/sn.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>

static sn_sv
sv_append(sn_sv this, sn_sv that)
{
    this.data = realloc(this.data, this.length + that.length);
    memcpy(this.data + this.length, that.data, that.length);
    this.length += that.length;

    return this;
}

sn_sv
ghcli_editor_get_user_message(void (*file_initializer)(FILE *, void *), void *user_data)
{
    const char *editor = getenv("EDITOR");
    if (!editor) {
        editor = ghcli_config_get_editor();
        if (!editor)
            errx(1, "No editor");
    }

    char   _filename[31] = "/tmp/ghcli_message.XXXXXXX\0";
    char * filename      = mktemp(_filename);

    FILE *file = fopen(filename, "w");
    file_initializer(file, user_data);
    fclose(file);

    pid_t pid = fork();
    if (pid == 0) {

        char *const argp[] = { (char *const)editor, filename, NULL };
        char *const envp[] = { NULL };

        if (execve(editor, argp, envp) < 0)
            err(1, "execve");
    } else {
        int status;
        if (waitpid(pid, &status, 0) < 0)
            err(1, "waitpid");

        if (!(WIFEXITED(status)))
            errx(1, "Editor child exited abnormally");

        if (WEXITSTATUS(status) != 0)
            errx(1, "Aborting PR. Editor command exited with code %d", WEXITSTATUS(status));
    }

    void *file_content = NULL;
    int len = sn_mmap_file(filename, &file_content);
    if (len < 0)
        err(1, "mmap");

    sn_sv result = {0};
    sn_sv buffer = sn_sv_from_parts(file_content, (size_t)len);
    buffer = sn_sv_trim_front(buffer);

    while (buffer.length > 0) {
        sn_sv line = sn_sv_chop_until(&buffer, '\n');

        if (buffer.length > 0) {
            buffer.length -= 1;
            buffer.data   += 1;
            line.length   += 1;
        }

        if (line.length > 0 && line.data[0] == '#')
            continue;

        result = sv_append(result, line);
        buffer = sn_sv_trim_front(buffer);
    }

    munmap(file_content, len);
    unlink(filename);

    return result;
}