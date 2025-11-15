/*
 * Copyright 2021-2025 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/cmd/cmd.h>
#include <gcli/cmd/cmdconfig.h>
#include <gcli/cmd/gitconfig.h>

#include <gcli/ctx.h>
#include <gcli/gcli.h>
#include <gcli/port/string.h>
#include <gcli/port/util.h>
#include <gcli/url.h>

#include <ctype.h>
#include <dirent.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_REMOTES 64
static struct gcli_gitremote remotes[MAX_REMOTES];
static size_t         remotes_size;

/* Resolve a worktree .git if needed */
static char *
resolve_worktree_gitdir_if_needed(char *dotgit)
{
	struct stat sb = {0};
	FILE *f;
	char *newdir = NULL;

	if (stat(dotgit, &sb) < 0)
		err(1, "gcli: stat");

	/* Real .git directory */
	if (S_ISDIR(sb.st_mode))
		return dotgit;

	f = fopen(dotgit, "r");
	if (!f)
		err(1, "gcli: fopen");

	while (!ferror(f) && !feof(f)) {
		char *key, *value;
		char buf[256] = {0};

		fgets(buf, sizeof buf, f);

		key = buf;
		value = strchr(buf, ':');
		if (!value)
			continue;

		*value++ = '\0';
		while (*value == ' ')
			++value;

		if (strcmp(key, "gitdir") == 0) {
			size_t const len = strlen(value);

			newdir = strdup(value);

			/* trim off any potential newline character */
			if (newdir[len - 1] == '\n')
				newdir[len - 1] = '\0';

			break;
		}
	}

	if (newdir == NULL)
		errx(1, "gcli: error: .git is a file but does not contain a gitdir pointer");

	fclose(f);

	/* In newer versions of git there is a file called "commondir"
	 * in .git in a worktree. It contains the path to the real
	 * .git directory */
	{
		char *other_path = gcli_asprintf("%s/%s", newdir, "commondir");
		if ((f = fopen(other_path, "r"))) {
			char buf[256] = {0};
			char *tmp = newdir;

			fgets(buf, sizeof buf, f);
			size_t const len = strlen(buf);
			if (buf[len-1] == '\n')
				buf[len-1] = '\0';

			newdir = gcli_asprintf("%s/%s", tmp, buf);
			free(tmp);

			fclose(f);
		}

		free(other_path);
	}

	free(dotgit);
	return newdir;
}

/* Search for a file named fname in the .git directory.
 *
 * This is ugly code. However, I don't see an easier way to do
 * this. */
static char *
find_file_in_dotgit(char const *fname)
{
	DIR           *curr_dir    = NULL;
	struct dirent *ent;
	char          *curr_dir_path;
	char          *dotgit      = NULL;
	char          *config_path = NULL;

	curr_dir_path = getcwd(NULL, 128);
	if (!curr_dir_path)
		err(1, "gcli: getcwd");

	/* Here we are trying to traverse upwards through the directory
	 * tree, searching for a directory called .git.
	 * Starting point is ".".*/
	do {
		curr_dir = opendir(curr_dir_path);
		if (!curr_dir)
			err(1, "gcli: opendir");

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

			curr_dir_path = gcli_cmd_realpath(tmp);
			if (!curr_dir_path)
				err(1, "gcli: error: realpath at %s", tmp);

			free(tmp);

			/* Check if we reached the filesystem root */
			if (strcmp("/", curr_dir_path) == 0) {
				free(curr_dir_path);
				closedir(curr_dir);
				gcli_warnx(g_clictx, "not a git repository");
				return NULL;
			}
		}


		closedir(curr_dir);
	} while (dotgit == NULL);

	free(curr_dir_path);

	/* In case we are working with git worktrees, the .git might be a
	 * file that contains a pointer to the actual .git directory. Here
	 * we call into a function that resolves this link if needed. */
	dotgit = resolve_worktree_gitdir_if_needed(dotgit);

	/* Now search for the file in the found .git directory */
	curr_dir = opendir(dotgit);
	if (!curr_dir)
		err(1, "gcli: opendir");

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

	errx(1, "gcli: error: .git without a config file");
	return NULL;
}

char *
gcli_find_gitconfig(void)
{
	return find_file_in_dotgit("config");
}

gcli_sv
gcli_gitconfig_get_current_branch(void)
{
	char const *HEAD;
	char       *file_text;
	gcli_sv     buffer;
	char        prefix[] = "ref: refs/heads/";

	HEAD = find_file_in_dotgit("HEAD");

	if (!HEAD)
		return SV_NULL;

	int len = gcli_read_file(HEAD, &file_text);
	if (len < 0)
		err(1, "gcli: mmap");

	buffer = gcli_sv_from_parts(file_text, len);

	if (gcli_sv_has_prefix(buffer, prefix)) {
		buffer.data   += sizeof(prefix) - 1;
		buffer.length -= sizeof(prefix) - 1;

		return gcli_sv_trim(buffer);
	} else {
		free(file_text);
		return SV_NULL;
	}
}

static void
parse_remote_url(struct gcli_gitremote *const remote)
{
	char *tmp;
	int rc = 0;
	size_t n = 0;
	struct gcli_url url = {0};

	rc = gcli_parse_url(remote->url, &url);
	if (rc < 0) {
		fprintf(stderr, "gcli: failed to parse remote url: %s. "
		        "This is probably a bug.\n", remote->url);

		goto bail;
	}

	/* automagic forge type */
	if (strcmp(url.host, "github.com") == 0)
		remote->forge_type = GCLI_FORGE_GITHUB;
	else if (strcmp(url.host, "gitlab.com") == 0)
		remote->forge_type = GCLI_FORGE_GITLAB;
	else if (strcmp(url.host, "codeberg.org") == 0)
		remote->forge_type = GCLI_FORGE_GITEA;

	tmp = strrchr(url.path, '/');
	if (tmp == NULL)
		goto bail;

	remote->owner = gcli_strndup(url.path, tmp - url.path);

	/* skip over '/' */
	tmp += 1;

	n = strlen(tmp);
	if (n > 4 && strcmp(tmp + (n - 4), ".git") == 0)
		n -= 4;

	remote->repo = gcli_strndup(tmp, n);

bail:
	gcli_url_free(&url);
}

static void
gitconfig_parse_remote(gcli_sv section_title, gcli_sv entry)
{
	gcli_sv remote_name = SV_NULL;

	/* If there is no remote name, just return and continue with the
	 * next section. I don't exactly know why there even are such
	 * sections and what they are useful for, but ok. */
	if (gcli_sv_eq_to(gcli_sv_trim(section_title), "remote"))
		return;

	/* the remote name is wrapped in double quotes */
	gcli_sv_chop_until(&section_title, '"');

	/* skip the first quote */
	section_title.data   += 1;
	section_title.length -= 1;

	remote_name = gcli_sv_chop_until(&section_title, '"');

	while ((entry = gcli_sv_trim_front(entry)).length > 0) {
		if (gcli_sv_has_prefix(entry, "url")) {
			if (remotes_size == MAX_REMOTES)
				errx(1, "gcli: error: too many remotes");

			struct gcli_gitremote *const remote = &remotes[remotes_size++];

			remote->name = gcli_sv_to_cstr(remote_name);

			gcli_sv_chop_until(&entry, '=');

			entry.data   += 1;
			entry.length -= 1;

			gcli_sv url = gcli_sv_trim(gcli_sv_chop_until(&entry, '\n'));

			remote->url = gcli_sv_to_cstr(url);
			remote->forge_type = -1;

			parse_remote_url(remote);
		} else {
			gcli_sv_chop_until(&entry, '\n');
		}
	}
}

static void
gcli_gitconfig_read_gitconfig(void)
{
	char *path = NULL;
	gcli_sv buffer = {0}, filebuf = {0};
	static int has_read_gitconfig = 0;

	if (has_read_gitconfig)
		return;

	has_read_gitconfig = 1;

	path = gcli_find_gitconfig();
	if (!path)
		return;

	filebuf.length = gcli_read_file(path, &filebuf.data);
	buffer = filebuf;

	free(path);
	path = NULL;

	while (buffer.length > 0) {
		buffer = gcli_sv_trim_front(buffer);

		if (buffer.length == 0)
			break;

		/* TODO: Git Config files support comments */
		if (*buffer.data != '[')
			errx(1, "gcli: error: invalid git config");

		gcli_sv section_title = gcli_sv_chop_until(&buffer, ']');
		section_title.length -= 1;
		section_title.data   += 1;

		buffer.length -= 2;
		buffer.data   += 2;

		gcli_sv entry = gcli_sv_chop_until(&buffer, '[');

		if (gcli_sv_has_prefix(section_title, "remote")) {
			gitconfig_parse_remote(section_title, entry);
		} else {
			// @@@: skip section
		}
	}

	free(filebuf.data);
	filebuf.length = 0;
	filebuf.data = NULL;
}

void
gcli_gitconfig_add_fork_remote(char const *org, char const *repo)
{
	char  remote[64]  = {0};
	FILE *remote_list = popen("git remote", "r");

	if (!remote_list)
		err(1, "gcli: popen");

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
					errx(1, "gcli: git child process failed");
			} else {
				err(1, "gcli: fork");
			}

			break;
		}
	}

	pclose(remote_list);

	/* Add new remote */
	{
		pid_t pid = 0;
		if ((pid = fork()) == 0) {
			char const *remote_url = gcli_asprintf(
				"git@github.com:%s/%s",
				org, repo);
			printf("[INFO] git remote add origin %s\n", remote_url);
			execlp("git", "git", "remote", "add", "origin", remote_url, NULL);
		} else if (pid > 0) {
			int status = 0;
			waitpid(pid, &status, 0);

			if (!(WIFEXITED(status) && (WEXITSTATUS(status) == 0)))
				errx(1, "gcli: git child process failed");
		} else {
			err(1, "gcli: fork");
		}
	}
}

/**
 * Return the gcli_forge_type for the given remote or -1 if
 * unknown */
int
gcli_gitconfig_get_forgetype(struct gcli_ctx *ctx, char const *const remote_name)
{
	(void) ctx;
	gcli_gitconfig_read_gitconfig();

	if (remote_name) {
		for (size_t i = 0; i < remotes_size; ++i) {
			if (strcmp(remotes[i].name, remote_name) == 0)
				return remotes[i].forge_type;
		}
	}

	if (!remotes_size) {
		gcli_warn(ctx, "no remotes to auto-detect forge");
		return -1;
	}

	return remotes[0].forge_type;
}

int
gcli_gitconfig_repo_by_remote(struct gcli_ctx *ctx, char const *const remote,
                              char **const owner, char **const repo,
                              int *const forge)
{
	gcli_gitconfig_read_gitconfig();

	if (remote) {
		for (size_t i = 0; i < remotes_size; ++i) {
			if (strcmp(remotes[i].name, remote) == 0) {
				*owner = remotes[i].owner;
				*repo  = remotes[i].repo;
				if (forge)
					*forge = remotes[i].forge_type;

				return 0;
			}
		}

		return gcli_error(ctx, "no such remote: %s", remote);
	}

	if (!remotes_size)
		return gcli_error(ctx, "no remotes to auto-detect forge");

	*owner = remotes[0].owner;
	*repo  = remotes[0].repo;
	if (forge)
		*forge = remotes[0].forge_type;

	return 0;
}

int
gcli_gitconfig_get_remote(struct gcli_ctx *ctx, gcli_forge_type const type,
                          char const **remote)
{
	gcli_gitconfig_read_gitconfig();

	for (size_t i = 0; i < remotes_size; ++i) {
		if (remotes[i].forge_type == type) {
			*remote = remotes[i].url;
			return 0;
		}
	}

	return gcli_error(ctx, "no suitable remote for forge type");
}
