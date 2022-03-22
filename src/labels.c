/*
 * Copyright 2022 Nico Sonack <nsonack@outlook.com>
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

#include <ghcli/color.h>
#include <ghcli/forges.h>
#include <ghcli/labels.h>

size_t
ghcli_get_labels(
    const char   *owner,
    const char   *reponame,
    int           max,
    ghcli_label **out)
{
    return ghcli_forge()->get_labels(owner, reponame, max, out);
}

void
ghcli_free_labels(ghcli_label *labels, size_t labels_size)
{
    for (size_t i = 0; i < labels_size; ++i) {
        free(labels[i].name);
        free(labels[i].description);
    }
    free(labels);
}

void
ghcli_print_labels(const ghcli_label *labels, size_t labels_size)
{
    printf("%10.10s %-15.15s %s\n", "ID", "NAME", "DESCRIPTION");

    for (size_t i = 0; i < labels_size; ++i) {
        printf(
            "%10.10ld %s%-15.15s%s %s\n",
            labels[i].id,
            ghcli_setcolor(labels[i].color), labels[i].name, ghcli_resetcolor(),
            labels[i].description);
    }
}

void
ghcli_create_label(const char *owner, const char *repo, ghcli_label *label)
{
    ghcli_forge()->create_label(owner, repo, label);
}

void
ghcli_delete_label(const char *owner, const char *repo, const char *label)
{
    ghcli_forge()->delete_label(owner, repo, label);
}
