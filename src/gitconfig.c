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

#include <ghcli/gitconfig.h>
#include <sn/sn.h>

#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char *
ghcli_find_gitconfig(void)
{
    DIR  *curr_dir = NULL;
    char *curr_dir_path;
    char *dotgit   = NULL;

    curr_dir_path = getcwd(NULL, 0);
    if (!curr_dir_path)
        err(1, "getcwd");

    /* Here we are trying to traverse upwards through the directory
     * tree, searching for a directory called .git.
     * Starting point is ".".*/
    do {
        curr_dir = opendir(curr_dir_path);
        if (!curr_dir)
            err(1, "opendir");

        struct dirent *ent;
        while ((ent = readdir(curr_dir))) {
            if (strcmp(".", ent->d_name) == 0 || strcmp("..", ent->d_name) == 0)
                continue;

            if (strcmp(".git", ent->d_name) == 0 && ent->d_namlen == 4) {
                size_t len = strlen(curr_dir_path);
                dotgit = malloc(len + ent->d_namlen + 2);
                memcpy(dotgit, curr_dir_path, len);
                dotgit[len] = '/';
                memcpy(dotgit + len + 1, ent->d_name, ent->d_namlen);

                dotgit[len + 1 + ent->d_namlen] = 0;

                break;
            }
        }

        if (!dotgit) {
            size_t len = strlen(curr_dir_path);
            char *tmp = malloc(len + sizeof("/..") + 1);

            memcpy(tmp, curr_dir_path, len);
            memcpy(tmp + len, "/..", sizeof("/..") + 1);

            free(curr_dir_path);

            curr_dir_path = realpath(tmp, NULL);
            if (!curr_dir_path)
                err(1, "realpath at %s", tmp);

            free(tmp);
        }


        closedir(curr_dir);
    } while (dotgit == NULL);

    return dotgit;
}
