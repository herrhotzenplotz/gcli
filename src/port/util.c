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

#include <gcli/port/util.h>
#include <gcli/port/sv.h>

#include <dirent.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

bool
gcli_yesno(const char *fmt, ...)
{
	char tmp = 0;
	va_list vp;
	gcli_sv message = {0};
	bool result = false;

	va_start(vp, fmt);
	message.length = vsnprintf(&tmp, 1, fmt, vp);
	va_end(vp);
	message.data = calloc(1, message.length + 1);

	va_start(vp, fmt);
	vsnprintf(message.data, message.length + 1, fmt, vp);
	va_end(vp);

	do {
		printf(SV_FMT" [yN] ", SV_ARGS(message));

		char c = getchar();

		if (c == 'y' || c == 'Y') {
			result = true;
			getchar();
			break;
		} else if (c == 'n' || c == 'N') {
			getchar();
			break;
		} else if (c == '\n') {
			break;
		}

		getchar(); // consume newline character

	} while (!feof(stdin));

	free(message.data);
	return result;
}

/* Read a file at path. '-' is stdin. This routine is usable for
 * non-seekable streams (such as stdin if it is a terminal). */
int
gcli_read_file(char const *path, char **buffer)
{
	FILE *f;
	size_t len, buflen;
	int rc = 0;

	if (strcmp(path, "-") == 0) {
		f = stdin;
	} else {
		f = fopen(path, "r");
		if (!f)
			return -1;
	}

	if (f == NULL)
		err(1, "failed to open file %s", path);

	/* read in the file */
	buflen = 8;
	len = 0;

	*buffer = malloc(buflen);

	while (!ferror(f) && !feof(f)) {
		int c = 0;

		if (len >= buflen) {
			buflen *= 2;
			*buffer = realloc(*buffer, buflen);
		}

		if ((c = fgetc(f)) == EOF)
			break;

		(*buffer)[len++] = c;
	}

	if (len >= buflen) {
		buflen += 1;
		*buffer = realloc(*buffer, buflen);
	}

	(*buffer)[len] = '\0';

	rc = (int)(len);

	if (f != stdin)
		fclose(f);

	return rc;
}

char *
gcli_find_directory(char const *dname)
{
	DIR *curr_dir = NULL;
	char *curr_dir_path = NULL;
	char *found_dir = NULL;
	struct dirent *ent = NULL;

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

			/* this is it? */
			if (strcmp(dname, ent->d_name) == 0) {
				size_t len = strlen(curr_dir_path);
				found_dir = malloc(len + strlen(ent->d_name) + 2);
				memcpy(found_dir, curr_dir_path, len);
				found_dir[len] = '/';
				memcpy(found_dir + len + 1, ent->d_name, strlen(ent->d_name));

				found_dir[len + 1 + strlen(ent->d_name)] = 0;

				break;
			}
		}

		/* not in this dir, traverse up */
		if (!found_dir) {
			size_t len = strlen(curr_dir_path);
			char *tmp = malloc(len + sizeof("/.."));

			memcpy(tmp, curr_dir_path, len);
			memcpy(tmp + len, "/..", sizeof("/.."));

			free(curr_dir_path);

			curr_dir_path = gcli_realpath(tmp);
			if (!curr_dir_path)
				err(1, "gcli: error: realpath at %s", tmp);

			free(tmp);

			/* Check if we reached the filesystem root */
			if (strcmp("/", curr_dir_path) == 0) {
				free(curr_dir_path);
				closedir(curr_dir);
				return NULL;
			}
		}

		closedir(curr_dir);
	} while (found_dir == NULL);

	free(curr_dir_path);

	return found_dir;
}

/* portability kludge for Slowlaris which to this day doesn't support
 * resolved_path to be NULL. */
char *
gcli_realpath(char const *const restrict pathname)
{
	char *resolved_path = NULL;

#if defined(_XOPEN_SOURCE) && _XOPEN_SOURCE < 700
	/* This system is certainly very old! Assume that PATH_MAX is defined.
	 * If not well there you go. Yes, this is flawed and ugly. */
	resolved_path = calloc(PATH_MAX, 1);
#endif

	return realpath(pathname, resolved_path);
}
