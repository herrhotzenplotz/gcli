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
#include <ghcli/gitconfig.h>
#include <ghcli/forges.h>

#include <ctype.h>
#include <dirent.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>

#define CONFIG_MAX_ENTRIES 16
struct ghcli_config_section {
    struct ghcli_config_entry {
        sn_sv key;
        sn_sv value;
    } entries[CONFIG_MAX_ENTRIES];
    size_t entries_size;

    sn_sv title;
};

#define CONFIG_MAX_SECTIONS 16
static struct ghcli_config {
    struct ghcli_config_section sections[CONFIG_MAX_SECTIONS];
    size_t                      sections_size;

    const char *override_default_account;
    const char *override_remote;

    sn_sv buffer;
    void *mmap_pointer;
} config;

static struct ghcli_dotghcli {
    struct ghcli_config_entry entries[128];
    size_t entries_size;

    sn_sv  buffer;
    void  *mmap_pointer;
    bool   has_been_searched_for;
    bool   has_been_found;
} local_config;

static bool
should_init_dotghcli(void)
{
    return !local_config.has_been_searched_for ||
        (local_config.has_been_searched_for && !local_config.has_been_found);
}

static const char *
find_dotghcli(void)
{
    char          *curr_dir_path = NULL;
    char          *dotghcli      = NULL;
    DIR           *curr_dir      = NULL;
    struct dirent *ent           = NULL;

    curr_dir_path = getcwd(NULL, 128);
    if (!curr_dir_path)
        err(1, "getcwd");

    /* Here we are trying to traverse upwards through the directory
     * tree, searching for a directory called .git.
     * Starting point is ".".*/
    do {
        curr_dir = opendir(curr_dir_path);
        if (!curr_dir)
            err(1, "opendir");

        while ((ent = readdir(curr_dir))) {
            if (strcmp(".", ent->d_name) == 0 || strcmp("..", ent->d_name) == 0)
                continue;

            if (strcmp(".ghcli", ent->d_name) == 0) {
                size_t len = strlen(curr_dir_path);
                dotghcli = malloc(len + strlen(ent->d_name) + 2);
                memcpy(dotghcli, curr_dir_path, len);
                dotghcli[len] = '/';
                memcpy(dotghcli + len + 1, ent->d_name, strlen(ent->d_name));

                dotghcli[len + 1 + strlen(ent->d_name)] = 0;

                break;
            }
        }


        if (!dotghcli) {
            size_t len = strlen(curr_dir_path);
            char *tmp = malloc(len + sizeof("/.."));

            memcpy(tmp, curr_dir_path, len);
            memcpy(tmp + len, "/..", sizeof("/.."));

            free(curr_dir_path);

            curr_dir_path = realpath(tmp, NULL);
            if (!curr_dir_path)
                err(1, "realpath at %s", tmp);

            free(tmp);

            if (strcmp("/", curr_dir_path) == 0) {
                free(curr_dir_path);
                closedir(curr_dir);

                // At this point we know for sure that we cannot find
                // a .ghcli and thus return a NULL pointer
                return NULL;
            }
        }


        closedir(curr_dir);
    } while (dotghcli == NULL);

    free(curr_dir_path);

    return dotghcli;
}

static void
init_local_config(void)
{
    if (!should_init_dotghcli())
        return;

    const char *path = find_dotghcli();
    if (!path) {
        local_config.has_been_searched_for = true;
        local_config.has_been_found        = false;
        return;
    }

    local_config.has_been_searched_for = true;
    local_config.has_been_found        = true;

    int len = sn_mmap_file(path, &local_config.mmap_pointer);
    if (len < 0)
        err(1, "Unable to open config file");

    local_config.buffer = sn_sv_from_parts(local_config.mmap_pointer, len);
    local_config.buffer = sn_sv_trim_front(local_config.buffer);

    int curr_line = 1;
    while (local_config.buffer.length > 0) {
        sn_sv line = sn_sv_chop_until(&local_config.buffer, '\n');

        line = sn_sv_trim(line);

        if (line.length == 0)
            errx(1, "%s:%d: Unexpected end of line",
                 path, curr_line);

        // Comments
        if (line.data[0] == '#') {
            local_config.buffer = sn_sv_trim_front(local_config.buffer);
            curr_line++;
            continue;
        }

        sn_sv key  = sn_sv_chop_until(&line, '=');

        key = sn_sv_trim(key);

        if (key.length == 0)
            errx(1, "%s:%d: empty key", path, curr_line);

        line.data   += 1;
        line.length -= 1;

        sn_sv value = sn_sv_trim(line);

        local_config.entries[local_config.entries_size].key   = key;
        local_config.entries[local_config.entries_size].value = value;
        local_config.entries_size++;

        local_config.buffer = sn_sv_trim_front(local_config.buffer);
        curr_line++;
    }

    free((void *)path);
}

static void
ghcli_config_parse_args(int *argc, char ***argv)
{
    /* These are the very first options passed to the ghcli command
     * itself. It is the first ever getopt call we do to parse any
     * arguments. Only global options that do not alter subcommand
     * specific behaviour should be accepted here. */

    int ch;
    const struct option options[] = {
        { .name    = "account",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'a' },
        { .name    = "remote",
          .has_arg = required_argument,
          .flag    = NULL,
          .val     = 'r' },
        {0},
    };

    while ((ch = getopt_long(*argc, *argv, "+a:r:", options, NULL)) != -1) {
        switch (ch) {
        case 'a': {
            config.override_default_account = optarg;
        } break;
        case 'r': {
            config.override_remote = optarg;
        } break;
        case '?':
        default:
            errx(1, "usage: ghcli [options] subcommand ...");
        }
    }

    *argc -= optind;
    *argv += optind;

    /* This one is a little odd: We are going to call getopt_long
     * again. Eventually. But since this is a global variable and the
     * getopt parser is reusing it, we need to reset it to zero. On
     * BSDs there is also the optreset variable, but it doesn't exist
     * on Solaris. I will thus not depend on it as it seems to be
     * working without it. */
    optind = 0;
}

struct config_parser {
    sn_sv       buffer;
    int         line;
    const char *filename;
};

static void
skip_ws_and_comments(struct config_parser *input)
{
again:
    while (input->buffer.length > 0) {
        switch (input->buffer.data[0]) {
        case '\n':
            input->line++;
        case ' ':
        case '\t':
        case '\r':
            input->buffer.data   += 1;
            input->buffer.length -= 1;
            break;
        default:
            goto not_whitespace;
        }
    }

    return;

not_whitespace:
    if (input->buffer.data[0] == '#') {
        /* This is a comment */
        sn_sv_chop_until(&input->buffer, '\n');
        goto again;
    }
}

static void
parse_keyvaluepair(struct config_parser *input, struct ghcli_config_entry *out)
{
    sn_sv key  = sn_sv_chop_until(&input->buffer, '=');

    if (key.length == 0)
        errx(1, "%s:%d: empty key", input->filename, input->line);

    input->buffer.data   += 1;
    input->buffer.length -= 1;

    sn_sv value = sn_sv_chop_until(&input->buffer, '\n');

    out->key   = sn_sv_trim(key);
    out->value = sn_sv_trim(value);
}

static sn_sv
parse_section_title(struct config_parser *input)
{
    size_t len = 0;
    if (input->buffer.length == 0)
        errx(1, "%s:%d: unexpected end of input in section title",
             input->filename, input->line);


    while (!isspace(input->buffer.data[len]) && input->buffer.data[len] != '{')
        len++;

    sn_sv title = sn_sv_from_parts(input->buffer.data, len);
    input->buffer.data   += len;
    input->buffer.length -= len;

    skip_ws_and_comments(input);

    if (input->buffer.length == 0)
        errx(1, "%s:%d: unexpected end of input", input->filename, input->line);

    if (input->buffer.data[0] != '{')
        errx(1, "%s:%d: expected '{'", input->filename, input->line);

    input->buffer.length -= 1;
    input->buffer.data   += 1;

    skip_ws_and_comments(input);
    return title;
}

static void
parse_config_section(struct config_parser *input)
{
    struct ghcli_config_section *section = NULL;

    if (config.sections_size == CONFIG_MAX_SECTIONS)
        errx(1, "error: too many config sections");

    section = &config.sections[config.sections_size++];

    section->title = parse_section_title(input);

    while (input->buffer.length > 0 && input->buffer.data[0] != '}') {
        skip_ws_and_comments(input);

        if (section->entries_size == CONFIG_MAX_ENTRIES)
            errx(1, "error: too many config entries in section "SV_FMT,
                 SV_ARGS(section->title));

        parse_keyvaluepair(input, &section->entries[section->entries_size++]);
        skip_ws_and_comments(input);
    }

    if (input->buffer.length == 0)
        errx(1, "%s:%d: missing '}' before end of file",
             input->filename, input->line);

    input->buffer.length -= 1;
    input->buffer.data   += 1;
}

static void
parse_config_file(struct config_parser *input)
{
    skip_ws_and_comments(input);

    while (input->buffer.length > 0) {
        parse_config_section(input);
        skip_ws_and_comments(input);
    }
}

void
ghcli_config_init(int *argc, char ***argv, const char *file_path)
{
    const char           *in_file_path = file_path;
    struct config_parser  parser       = {0};

    ghcli_config_parse_args(argc, argv);

    if (!file_path) {
        file_path = getenv("XDG_CONFIG_PATH");
        if (!file_path) {
            file_path = getenv("HOME");
            if (!file_path)
                errx(1, "Neither XDG_CONFIG_PATH nor HOME set in env");

            /*
             * Code duplication to avoid leaking pointers */
            file_path = sn_asprintf("%s/.config/ghcli/config", file_path);
        } else {
            file_path = sn_asprintf("%s/ghcli/config", file_path);
        }
    }

    if (access(file_path, R_OK) < 0)
        err(1, "Cannot access config file at %s", file_path);

    int len = sn_mmap_file(file_path, &config.mmap_pointer);
    if (len < 0)
        err(1, "Unable to open config file");

    config.buffer = sn_sv_from_parts(config.mmap_pointer, len);
    config.buffer = sn_sv_trim_front(config.buffer);

    parser.buffer   = config.buffer;
    parser.line     = 1;
    parser.filename = file_path;

    parse_config_file(&parser);

    if (file_path != in_file_path)
        free((void *)file_path);
}

static struct ghcli_config_section *
find_section(sn_sv name)
{
    for (size_t i = 0; i < config.sections_size; ++i) {
        if (sn_sv_eq(config.sections[i].title, name))
            return &config.sections[i];
    }
    return NULL;
}

sn_sv
ghcli_config_find_by_key(sn_sv section_name, const char *key)
{
    struct ghcli_config_section *section = find_section(section_name);

    if (!section) {
        warnx("no config section with name '"SV_FMT"'", SV_ARGS(section_name));
        return SV_NULL;
    }

    for (size_t i = 0; i < section->entries_size; ++i)
        if (sn_sv_eq_to(section->entries[i].key, key))
            return section->entries[i].value;

    return SV_NULL;
}

static sn_sv
ghcli_local_config_find_by_key(const char *key)
{
    for (size_t i = 0; i < local_config.entries_size; ++i)
        if (sn_sv_eq_to(local_config.entries[i].key, key))
            return local_config.entries[i].value;
    return SV_NULL;
}

char *
ghcli_config_get_editor(void)
{
    return sn_sv_to_cstr(ghcli_config_find_by_key(SV("defaults"), "editor"));
}

char *
ghcli_config_get_authheader(void)
{
    return ghcli_forge()->get_authheader();
}

sn_sv
ghcli_config_get_account(void)
{
    return ghcli_forge()->get_account();
}

sn_sv
ghcli_config_get_upstream(void)
{
    init_local_config();

    return ghcli_local_config_find_by_key("pr.upstream");
}

void
ghcli_config_get_upstream_parts(sn_sv *owner, sn_sv *repo)
{
    sn_sv upstream   = ghcli_config_get_upstream();
    *owner           = sn_sv_chop_until(&upstream, '/');
    /* TODO: Sanity check */
    upstream.data   += 1;
    upstream.length -= 1;
    *repo            = upstream;
}

sn_sv
ghcli_config_get_base(void)
{
    init_local_config();

    return ghcli_local_config_find_by_key("pr.base");
}

sn_sv
ghcli_config_get_override_default_account(void)
{
    init_local_config();

    if (config.override_default_account)
        return SV((char *)config.override_default_account);
    else
        return SV_NULL;
}

ghcli_forge_type
ghcli_config_get_forge_type(void)
{
    init_local_config();

    sn_sv entry = {0};

    if (config.override_default_account) {
        sn_sv section = SV((char *)config.override_default_account);
        entry = ghcli_config_find_by_key(section, "forge-type");
        if (sn_sv_null(entry))
            errx(1,
                 "error: given default override account not found or "
                 "missing forge-type");
    } else {
        entry = ghcli_local_config_find_by_key("forge-type");
    }

    if (!sn_sv_null(entry)) {
        if (sn_sv_eq_to(entry, "github"))
            return GHCLI_FORGE_GITHUB;
        else if (sn_sv_eq_to(entry, "gitlab"))
            return GHCLI_FORGE_GITLAB;
        else
            errx(1, "Unknown forge type "SV_FMT, SV_ARGS(entry));
    }

    return ghcli_gitconfig_get_forgetype(config.override_remote);
}

void
ghcli_config_get_repo(const char **owner, const char **repo)
{
    sn_sv upstream = {0};

    if (config.override_remote) {
        ghcli_forge_type forge = ghcli_gitconfig_repo_by_remote(
            config.override_remote, owner, repo);

        if (forge >= 0) {
            if (ghcli_config_get_forge_type() != forge)
                errx(1, "error: forge types are inconsistent");
        }

        return;
    }

    if ((upstream = ghcli_config_get_upstream()).length != 0) {
        sn_sv owner_sv = sn_sv_chop_until(&upstream, '/');
        sn_sv repo_sv  = sn_sv_from_parts(
            upstream.data + 1,
            upstream.length - 1);

        *owner = sn_sv_to_cstr(owner_sv);
        *repo  = sn_sv_to_cstr(repo_sv);

        return;
    }

    ghcli_gitconfig_repo_by_remote(NULL, owner, repo);
}
