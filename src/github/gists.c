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

#include <gcli/cmd/colour.h>
#include <gcli/cmd/table.h>
#include <gcli/config.h>
#include <gcli/curl.h>
#include <gcli/github/gists.h>
#include <gcli/json_util.h>

#include <gcli/github/config.h>

#include <pdjson/pdjson.h>

#include <templates/github/gists.h>

/* /!\ Before changing this, see comment in gists.h /!\ */
int
parse_github_gist_files_idiot_hack(gcli_ctx *ctx, json_stream *stream,
                                   gcli_gist *const gist)
{
	(void) ctx;

	enum json_type next = JSON_NULL;

	gist->files = NULL;
	gist->files_size = 0;

	if ((next = json_next(stream)) != JSON_OBJECT)
		return gcli_error(ctx, "expected Gist Files Object");

	while ((next = json_next(stream)) == JSON_STRING) {
		gist->files = realloc(gist->files, sizeof(*gist->files) * (gist->files_size + 1));
		gcli_gist_file *it = &gist->files[gist->files_size++];
		if (parse_github_gist_file(ctx, stream, it) < 0)
			return -1;
	}

	if (next != JSON_OBJECT_END)
		return gcli_error(ctx, "unclosed Gist Files Object");

	return 0;
}

int
gcli_get_gists(gcli_ctx *ctx, char const *user, int const max,
               gcli_gist_list *const list)
{
	char *url = NULL;
	gcli_fetch_list_ctx fl = {
		.listp = &list->gists,
		.sizep = &list->gists_size,
		.parse = (parsefn)(parse_github_gists),
		.max = max,
	};

	if (user)
		url = sn_asprintf(
			"%s/users/%s/gists",
			github_get_apibase(ctx),
			user);
	else
		url = sn_asprintf("%s/gists", github_get_apibase(ctx));

	return gcli_fetch_list(ctx, url, &fl);
}

int
gcli_get_gist(gcli_ctx *ctx, char const *gist_id, gcli_gist *out)
{
	char *url = NULL;
	gcli_fetch_buffer buffer = {0};
	int rc = 0;

	url = sn_asprintf("%s/gists/%s", github_get_apibase(ctx), gist_id);
	rc = gcli_fetch(ctx, url, NULL, &buffer);

	if (rc == 0) {
		struct json_stream  stream = {0};

		json_open_buffer(&stream, buffer.data, buffer.length);
		json_set_streaming(&stream, 1);

		parse_github_gist(ctx, &stream, out);

		json_close(&stream);
	}

	free(buffer.data);
	free(url);

	return rc;
}

#define READ_SZ 4096
static size_t
read_file(FILE *f, char **out)
{
	size_t size = 0;

	*out = NULL;

	while (!feof(f) && !ferror(f)) {
		*out = realloc(*out, size + READ_SZ);
		size_t bytes_read = fread(*out + size, 1, READ_SZ, f);
		if (bytes_read == 0)
			break;
		size += bytes_read;
	}

	return size;
}

int
gcli_create_gist(gcli_ctx *ctx, gcli_new_gist opts)
{
	char *url = NULL;
	char *post_data = NULL;
	gcli_fetch_buffer fetch_buffer = {0};
	sn_sv read_buffer = {0};
	sn_sv content = {0};
	int rc = 0;

	read_buffer.length = read_file(opts.file, &read_buffer.data);
	content = gcli_json_escape(read_buffer);

	/* This API is documented very badly. In fact, I dug up how you're
	 * supposed to do this from
	 * https://github.com/phadej/github/blob/master/src/GitHub/Data/Gists.hs
	 *
	 * From this we can infer that we're supposed to create a JSON
	 * object like so:
	 *
	 * {
	 *  "description": "foobar",
	 *  "public": true,
	 *  "files": {
	 *   "barf.exe": {
	 *       "content": "#!/bin/sh\necho This file cannot be run in DOS mode"
	 *   }
	 *  }
	 * }
	 */

	/* TODO: Escape gist_description and file_name */
	url = sn_asprintf("%s/gists", github_get_apibase(ctx));
	post_data = sn_asprintf(
		"{\"description\":\"%s\",\"public\":true,\"files\":"
		"{\"%s\": {\"content\":\""SV_FMT"\"}}}",
		opts.gist_description,
		opts.file_name,
		SV_ARGS(content));

	rc = gcli_fetch_with_method(ctx, "POST", url, post_data, NULL, &fetch_buffer);

	free(read_buffer.data);
	free(fetch_buffer.data);
	free(url);
	free(post_data);

	return rc;
}

int
gcli_delete_gist(gcli_ctx *ctx, char const *gist_id)
{
	char *url = NULL;
	gcli_fetch_buffer buffer = {0};
	int rc = 0;

	url = sn_asprintf(
		"%s/gists/%s",
		github_get_apibase(ctx),
		gist_id);

	rc = gcli_fetch_with_method(ctx, "DELETE", url, NULL, NULL, &buffer);

	free(buffer.data);
	free(url);

	return rc;
}

void
gcli_gist_free(gcli_gist *g)
{
	free(g->id.data);
	free(g->owner.data);
	free(g->url.data);
	free(g->date.data);
	free(g->git_pull_url.data);
	free(g->description.data);

	for (size_t j = 0; j < g->files_size; ++j) {
		free(g->files[j].filename.data);
		free(g->files[j].language.data);
		free(g->files[j].url.data);
		free(g->files[j].type.data);
	}

	free(g->files);

	memset(g, 0, sizeof(*g));
}

void
gcli_gists_free(gcli_gist_list *const list)
{
	for (size_t i = 0; i < list->gists_size; ++i)
		gcli_gist_free(&list->gists[i]);

	free(list->gists);

	list->gists = NULL;
	list->gists_size = 0;
}
