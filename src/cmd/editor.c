/*
 * Copyright Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/cmd/cmdconfig.h>
#include <gcli/cmd/editor.h>
#include <gcli/port/err.h>
#include <gcli/port/util.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>

static char *
get_env_editor(void)
{
	static char const *const env_vars[] = { "GIT_EDITOR", "VISUAL", "EDITOR", NULL };
	size_t i = 0;
	char *result = NULL;
	do
		result = getenv(env_vars[i]);
	while (!result && env_vars[++i]);
	return result;
}

static void
edit(struct gcli_ctx *ctx, char const *filename)
{
	char const *editor = get_env_editor();

	if (!editor) {
		editor = gcli_config_get_editor(ctx);
		if (!editor)
			errx(1,
			     "I have no editor. Either set editor=... in your config "
			     "file or set the EDITOR environment variable.");
	}

	pid_t pid = fork();
	if (pid == 0) {
		if (execlp(editor, editor, filename, NULL) < 0)
			err(1, "execlp");
	} else {
		int status;
		if (waitpid(pid, &status, 0) < 0)
			err(1, "waitpid");

		if (!(WIFEXITED(status)))
			errx(1, "Editor child exited abnormally");

		if (WEXITSTATUS(status) != 0)
			errx(1, "Aborting. Editor command exited with code %d",
			     WEXITSTATUS(status));
	}
}

char *
gcli_editor_get_user_message(
	struct gcli_ctx *ctx,
	void (*file_initializer)(struct gcli_ctx *, FILE *, void *),
	void *user_data)
{
	char filename[31] = "/tmp/gcli_message.XXXXXXX\0";
	int fd = mkstemp(filename);
	FILE *file = fdopen(fd, "w");

	file_initializer(ctx, file, user_data);
	fclose(file);

	edit(ctx, filename);

	char *file_content = NULL;
	int len = gcli_read_file(filename, &file_content);
	if (len < 0)
		err(1, "read_file");

	gcli_sv result = {0};
	gcli_sv buffer = gcli_sv_from_parts(file_content, (size_t)len);
	buffer = gcli_sv_trim_front(buffer);

	while (buffer.length > 0) {
		gcli_sv line = gcli_sv_chop_until(&buffer, '\n');

		if (buffer.length > 0) {
			buffer.length -= 1;
			buffer.data   += 1;
			line.length   += 1;
		}

		if (line.length > 0 && line.data[0] == '!')
			continue;

		result = gcli_sv_append(result, line);
	}

	free(file_content);
	unlink(filename);

	/* When the input is empty, the data pointer is going to be NULL.
	 * Do not access it in this case. */
	if (result.length)
		result.data[result.length] = '\0';

	return result.data;
}

int
gcli_editor_open_file(struct gcli_ctx *ctx, char const *const path)
{
	edit(ctx, path);
	return 0;
}
