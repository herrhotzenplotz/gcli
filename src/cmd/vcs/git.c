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

#include <gcli/cmd/vcs/git.h>

#include <gcli/cmd/cmd.h>
#include <gcli/cmd/vcs.h>
#include <gcli/cmd/cmdconfig.h>

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


/* Search for a file named fname in the .git directory. */
static char *
find_file_in_dotgit(char const *fname)
{
	char *config_path = NULL;
	char *dotgit = NULL;
	size_t fname_len, config_path_len;

	dotgit = gcli_find_directory(".git");
	if (!dotgit) {
		gcli_warnx(g_clictx, "not a git repository");
		return NULL;
	}

	/* In case we are working with git worktrees, the .git might be a
	 * file that contains a pointer to the actual .git directory. Here
	 * we call into a function that resolves this link if needed. */
	dotgit = resolve_worktree_gitdir_if_needed(dotgit);

	/* Now search for the file in the found .git directory */
	fname_len = strlen(fname);
	config_path_len = strlen(dotgit) + 1 + fname_len + 1;

	config_path = calloc(1, config_path_len);
	snprintf(config_path, config_path_len, "%s/%s", dotgit, fname);

	if (access(config_path, F_OK) < 0)
		errx(1, "gcli: error: .git without a config file");

	free(dotgit);

	return config_path;
}

char *
gcli_find_gitconfig(void)
{
	return find_file_in_dotgit("config");
}

int
gcli_vcs_git_get_current_branch(struct gcli_ctx *ctx, char **out)
{
	char *file_text;
	char const *HEAD;
	char prefix[] = "ref: refs/heads/";
	gcli_sv buffer;

	(void) ctx;

	HEAD = find_file_in_dotgit("HEAD");

	if (!HEAD)
		return -1;

	int len = gcli_read_file(HEAD, &file_text);
	if (len < 0)
		err(1, "gcli: mmap");

	buffer = gcli_sv_from_parts(file_text, len);

	if (gcli_sv_has_prefix(buffer, prefix)) {
		buffer.data   += sizeof(prefix) - 1;
		buffer.length -= sizeof(prefix) - 1;

		*out = gcli_sv_to_cstr(gcli_sv_trim(buffer));
		return 0;
	} else {
		free(file_text);
		return -1;
	}
}

static int
parse_remote_url(struct gcli_cmd_vcs_remote *const remote, char const *url_text)
{
	char *tmp;
	int rc = 0;
	size_t n = 0;
	struct gcli_url url = {0};

	rc = gcli_parse_url(url_text, &url);
	if (rc < 0) {
		fprintf(stderr, "gcli: failed to parse remote url: %s. "
		        "This is probably a bug.\n", url_text);

		goto bail;
	}

	/* probably a local clone */
	if (!url.host) {
		rc = 0;
		goto bail;
	}

	/* save away the host */
	remote->host = strdup(url.host);

	/* automagic forge type */
	gcli_vcs_guess_forgetype_by_hostname(url.host, &remote->forge_type);

	/* split owner/repo */
	tmp = strrchr(url.path, '/');
	if (tmp == NULL) {
		rc = -1;
		goto bail;
	}

	remote->owner = gcli_strndup(url.path, tmp - url.path);

	/* skip over '/' */
	tmp += 1;

	n = strlen(tmp);
	if (n > 4 && strcmp(tmp + (n - 4), ".git") == 0)
		n -= 4;

	remote->repo = gcli_strndup(tmp, n);
	rc = 0;

bail:
	gcli_url_free(&url);
	return rc;
}

static void
gitconfig_parse_remote(struct gcli_cmd_vcs_remotes *remotes,
                       gcli_sv section_title, gcli_sv entry)
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
			char *url;

			struct gcli_cmd_vcs_remote *const r =
				calloc(1, sizeof(*r));

			r->name = gcli_sv_to_cstr(remote_name);

			gcli_sv_chop_until(&entry, '=');

			entry.data   += 1;
			entry.length -= 1;

			url = gcli_sv_to_cstr(
				gcli_sv_trim(
					gcli_sv_chop_until(&entry, '\n')
				)
			);

			r->forge_type = -1;

			parse_remote_url(r, url);
			free(url);

			TAILQ_INSERT_TAIL(remotes, r, next);
		} else {
			gcli_sv_chop_until(&entry, '\n');
		}
	}
}

int
gcli_vcs_git_read_repoconfig(struct gcli_ctx *ctx,
                             struct gcli_cmd_vcs_remotes *remotes)
{
	char *path = NULL;
	gcli_sv buffer = {0}, filebuf = {0};

	(void) ctx;

	path = gcli_find_gitconfig();
	if (!path)
		return 0;

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
			gitconfig_parse_remote(remotes, section_title, entry);
		} else {
			// @@@: skip section
		}
	}

	free(filebuf.data);
	filebuf.length = 0;
	filebuf.data = NULL;

	return 0;
}

void
gcli_vcs_git_add_fork_remote(char const *org, char const *repo)
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
