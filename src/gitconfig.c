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

#include <gcli/config.h>
#include <gcli/gcli.h>
#include <gcli/gitconfig.h>
#include <sn/sn.h>

#include <stdio.h>
#include <sys/wait.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <sys/mman.h>

#define MAX_REMOTES 64
static gcli_gitremote remotes[MAX_REMOTES];
static size_t         remotes_size;

/* Search for a file named fname in the .gcli directory.
 *
 * This is ugly code. However, I don't see an easier way to do
 * this. */
static char const *
find_file_in_dotgit(char const *fname)
{
	DIR           *curr_dir    = NULL;
	struct dirent *ent;
	char          *curr_dir_path;
	char          *dotgit      = NULL;
	char          *config_path = NULL;

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

		/* Read entries of the directory */
		while ((ent = readdir(curr_dir))) {
			if (strcmp(".", ent->d_name) == 0 || strcmp("..", ent->d_name) == 0)
				continue;

			/* Is this the .git directory? If so, allocate some memory
			 * to store the path into dotgit and append '/\0' */
			if (strcmp(".git", ent->d_name) == 0) {
				size_t len = strlen(curr_dir_path);
				dotgit = malloc(len + strlen(ent->d_name) + 2);
				memcpy(dotgit, curr_dir_path, len);
				dotgit[len] = '/';
				memcpy(dotgit + len + 1, ent->d_name, strlen(ent->d_name));

				dotgit[len + 1 + strlen(ent->d_name)] = 0;

				break;
			}
		}

		/* If we reach this point and dotgit is NULL we couldn't find
		 * the .git directory in the current directory. In this case
		 * we append '..' to the path and resolve it. */
		if (!dotgit) {
			size_t len = strlen(curr_dir_path);
			char *tmp = malloc(len + sizeof("/.."));

			memcpy(tmp, curr_dir_path, len);
			memcpy(tmp + len, "/..", sizeof("/.."));

			free(curr_dir_path);

			curr_dir_path = realpath(tmp, NULL);
			if (!curr_dir_path)
				err(1, "error: realpath at %s", tmp);

			free(tmp);

			/* Check if we reached the filesystem root */
			if (strcmp("/", curr_dir_path) == 0) {
				free(curr_dir_path);
				closedir(curr_dir);
				warnx("not a git repository");
				return NULL;
			}
		}


		closedir(curr_dir);
	} while (dotgit == NULL);

	free(curr_dir_path);

	/* Now search for the file in the found .git directory */
	curr_dir = opendir(dotgit);
	if (!curr_dir)
		err(1, "opendir");

	while ((ent = readdir(curr_dir))) {
		/* skip over . and .. directory entries */
		if (strcmp(".", ent->d_name) == 0 || strcmp("..", ent->d_name) == 0)
			continue;

		/* We found the config file, put together it's path and return
		 * that */
		if (strcmp(fname, ent->d_name) == 0) {
			int len = strlen(dotgit);

			config_path = malloc(len + 1 + sizeof(fname));

			memcpy(config_path, dotgit, len);
			config_path[len] = '/';

			memcpy(config_path + len + 1, fname, strlen(fname) + 1);

			closedir(curr_dir);
			free(dotgit);

			return config_path;
		}
	}

	errx(1, "error: .git without a config file");
	return NULL;
}

char const *
gcli_find_gitconfig(void)
{
	return find_file_in_dotgit("config");
}

sn_sv
gcli_gitconfig_get_current_branch(void)
{
	char const *HEAD;
	void       *mmap_pointer;
	sn_sv       buffer;
	char        prefix[] = "ref: refs/heads/";

	HEAD = find_file_in_dotgit("HEAD");

	if (!HEAD)
		return SV_NULL;

	int len = sn_mmap_file(HEAD, &mmap_pointer);
	if (len < 0)
		err(1, "mmap");

	buffer = sn_sv_from_parts(mmap_pointer, len);

	if (sn_sv_has_prefix(buffer, prefix)) {
		buffer.data   += sizeof(prefix) - 1;
		buffer.length -= sizeof(prefix) - 1;

		return sn_sv_trim(buffer);
	} else {
		munmap(mmap_pointer, len);
		return SV_NULL;
	}
}

static void
http_extractor(gcli_gitremote *const remote, char const *prefix)
{
	size_t prefix_size = strlen(prefix);
	sn_sv  pair        = remote->url;

	if (sn_sv_has_prefix(remote->url, "https://github.com/")) {
		prefix_size = sizeof("https://github.com/") - 1;
		remote->forge_type = GCLI_FORGE_GITHUB;
	} else if (sn_sv_has_prefix(remote->url, "https://gitlab.com/")) {
		prefix_size = sizeof("https://gitlab.com/") - 1;
		remote->forge_type = GCLI_FORGE_GITLAB;
	} else if (sn_sv_has_prefix(remote->url, "https://codeberg.org/")) {
		prefix_size = sizeof("https://codeberg.org/") - 1;
		remote->forge_type = GCLI_FORGE_GITEA;
	} else {
		warnx("non-github, non-gitlab and non-codeberg https remotes are "
		      "not supported and will likely cause bugs");
	}

	pair.length -= prefix_size;
	pair.data   += prefix_size;

	remote->owner = sn_sv_chop_until(&pair, '/');

	pair.data   += 1;
	pair.length -= 1;

	pair = sn_sv_strip_suffix(pair, ".git");

	remote->repo = pair;
}

static void
ssh_extractor(gcli_gitremote *const remote, char const *prefix)
{
	size_t prefix_size = strlen(prefix);

	if (sn_sv_has_prefix(remote->url, "git@github.com"))
		remote->forge_type = GCLI_FORGE_GITHUB;
	else if (sn_sv_has_prefix(remote->url, "git@gitlab.com"))
		remote->forge_type = GCLI_FORGE_GITLAB;
	else if (sn_sv_has_prefix(remote->url, "git@codeberg.org"))
		remote->forge_type = GCLI_FORGE_GITEA;

	sn_sv pair   = remote->url;
	pair.length -= prefix_size;
	pair.data   += prefix_size;

	sn_sv_chop_until(&pair, ':');
	pair.data   += 1;
	pair.length -= 1;

	remote->owner = sn_sv_chop_until(&pair, '/');

	pair.data   += 1;
	pair.length -= 1;

	pair = sn_sv_strip_suffix(pair, ".git");

	remote->repo = pair;
}

struct forge_ex_def {
	char const *prefix;
	void (*extractor)(gcli_gitremote *const, char const *);
} url_extractors[] = {
	{ .prefix = "git@",     .extractor = ssh_extractor  },
	{ .prefix = "ssh://",   .extractor = ssh_extractor  },
	{ .prefix = "https://", .extractor = http_extractor },
};

static void
gitconfig_parse_remote(sn_sv section_title, sn_sv entry)
{
	sn_sv remote_name = SV_NULL;

	/* If there is no remote name, just return and continue with the
	 * next section. I don't exactly know why there even are such
	 * sections and what they are useful for, but ok. */
	if (sn_sv_eq_to(sn_sv_trim(section_title), "remote"))
		return;

	/* the remote name is wrapped in double quotes */
	sn_sv_chop_until(&section_title, '"');

	/* skip the first quote */
	section_title.data   += 1;
	section_title.length -= 1;

	remote_name = sn_sv_chop_until(&section_title, '"');

	while ((entry = sn_sv_trim_front(entry)).length > 0) {
		if (sn_sv_has_prefix(entry, "url")) {
			if (remotes_size == MAX_REMOTES)
				errx(1, "error: too many remotes");

			gcli_gitremote *const remote = &remotes[remotes_size++];

			remote->name = remote_name;

			sn_sv_chop_until(&entry, '=');

			entry.data   += 1;
			entry.length -= 1;

			sn_sv url = sn_sv_trim(sn_sv_chop_until(&entry, '\n'));

			remote->url        = url;
			remote->forge_type = -1;

			for (size_t i = 0; i < ARRAY_SIZE(url_extractors); ++i) {
				if (sn_sv_has_prefix(url, url_extractors[i].prefix)) {
					url_extractors[i].extractor(
						remote,
						url_extractors[i].prefix);
				}
			}
		} else {
			sn_sv_chop_until(&entry, '\n');
		}
	}
}

static void
gcli_gitconfig_read_gitconfig(void)
{
	static int has_read_gitconfig = 0;

	if (has_read_gitconfig)
		return;

	has_read_gitconfig = 1;

	char const *path   = NULL;
	sn_sv       buffer = {0};

	path = gcli_find_gitconfig();
	if (!path)
		return;

	buffer.length = sn_mmap_file(path, (void **)&buffer.data);

	while (buffer.length > 0) {
		buffer = sn_sv_trim_front(buffer);

		if (buffer.length == 0)
			break;

		/* TODO: Git Config files support comments */
		if (*buffer.data != '[')
			errx(1, "error: invalid git config");

		sn_sv section_title = sn_sv_chop_until(&buffer, ']');
		section_title.length -= 1;
		section_title.data   += 1;

		buffer.length -= 2;
		buffer.data   += 2;

		sn_sv entry = sn_sv_chop_until(&buffer, '[');

		if (sn_sv_has_prefix(section_title, "remote")) {
			gitconfig_parse_remote(section_title, entry);
		} else {
			// @@@: skip section
		}
	}
}

void
gcli_gitconfig_add_fork_remote(char const *org, char const *repo)
{
	char  remote[64]  = {0};
	FILE *remote_list = popen("git remote", "r");

	if (!remote_list)
		err(1, "popen");

	/* TODO: Output informational messages */
	/* Rename possibly existing origin remote to point at the
	 * upstream */
	while (fgets(remote, sizeof(remote), remote_list)) {
		if (strcmp(remote, "origin\n") == 0) {

			pid_t pid = 0;
			if ((pid = fork()) == 0) {
				printf("[INFO] git remote rename origin upstream\n");
				execlp("git", "git", "remote",
				       "rename", "origin", "upstream", NULL);
			} else if (pid > 0) {
				int status = 0;
				waitpid(pid, &status, 0);

				if (!(WIFEXITED(status) && (WEXITSTATUS(status) == 0)))
					errx(1, "git child process failed");
			} else {
				err(1, "fork");
			}

			break;
		}
	}

	pclose(remote_list);

	/* Add new remote */
	{
		pid_t pid = 0;
		if ((pid = fork()) == 0) {
			char const *remote_url = sn_asprintf(
				"git@github.com:%s/%s",
				org, repo);
			printf("[INFO] git remote add origin %s\n", remote_url);
			execlp("git", "git", "remote", "add", "origin", remote_url, NULL);
		} else if (pid > 0) {
			int status = 0;
			waitpid(pid, &status, 0);

			if (!(WIFEXITED(status) && (WEXITSTATUS(status) == 0)))
				errx(1, "git child process failed");
		} else {
			err(1, "fork");
		}
	}
}

/**
 * Return the gcli_forge_type for the given remote or -1 if
 * unknown */
int
gcli_gitconfig_get_forgetype(char const *const remote_name)
{
	gcli_gitconfig_read_gitconfig();

	if (remote_name) {
		for (size_t i = 0; i < remotes_size; ++i) {
			if (sn_sv_eq_to(remotes[i].name, remote_name))
				return remotes[i].forge_type;
		}
	}

	if (!remotes_size) {
		warn("no remotes to auto-detect forge");
		return -1;
	}

	return remotes[0].forge_type;
}

int
gcli_gitconfig_repo_by_remote(
	char const *const  remote_name,
	char const **const owner,
	char const **const repo)
{
	gcli_gitconfig_read_gitconfig();

	if (remote_name) {
		for (size_t i = 0; i < remotes_size; ++i) {
			if (sn_sv_eq_to(remotes[i].name, remote_name)) {
				*owner = sn_sv_to_cstr(remotes[i].owner);
				*repo  = sn_sv_to_cstr(remotes[i].repo);
				return remotes[i].forge_type;
			}
		}

		errx(1, "error: no such remote: %s", remote_name);
	}

	if (!remotes_size)
		errx(1, "error: no remotes to auto-detect forge");

	*owner = sn_sv_to_cstr(remotes[0].owner);
	*repo  = sn_sv_to_cstr(remotes[0].repo);
	return remotes[0].forge_type;
}
