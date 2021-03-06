/*
 * Copyright 2021 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/gists.h>
#include <gcli/config.h>
#include <gcli/color.h>
#include <gcli/curl.h>
#include <gcli/json_util.h>

#include <gcli/github/config.h>

#include <pdjson/pdjson.h>

static void
parse_gist_file(struct json_stream *stream, gcli_gist_file *file)
{
	enum json_type  next       = JSON_NULL;
	enum json_type  value_type = JSON_NULL;
	const char     *key;

	if ((next = json_next(stream)) != JSON_OBJECT)
		errx(1, "Expected Gist File Object");

	while ((next = json_next(stream)) == JSON_STRING) {
		size_t len;
		key = json_get_string(stream, &len);

		if (strncmp("filename", key, len) == 0) {
			file->filename = get_sv(stream);
		} else if (strncmp("language", key, len) == 0) {
			file->language = get_sv(stream);
		} else if (strncmp("raw_url", key, len) == 0) {
			file->url = get_sv(stream);
		} else if (strncmp("size", key, len) == 0) {
			file->size = get_int(stream);
		} else if (strncmp("type", key, len) == 0) {
			file->type = get_sv(stream);
		} else {
			value_type = json_next(stream);

			switch (value_type) {
			case JSON_ARRAY:
				json_skip_until(stream, JSON_ARRAY_END);
				break;
			case JSON_OBJECT:
				json_skip_until(stream, JSON_OBJECT_END);
				break;
			default:
				break;
			}
		}
	}

	if (next != JSON_OBJECT_END)
		errx(1, "Unclosed Gist File Object");
}

static void
parse_gist_files(
	struct json_stream  *stream,
	gcli_gist_file    **files,
	size_t              *files_size)
{
	enum json_type next     = JSON_NULL;
	size_t         size     = 0;

	*files = NULL;

	if ((next = json_next(stream)) != JSON_OBJECT)
		errx(1, "Expected Gist Files Object");

	while ((next = json_next(stream)) == JSON_STRING) {
		*files = realloc(*files, sizeof(gcli_gist_file) * (size + 1));
		gcli_gist_file *it = &(*files)[size++];
		parse_gist_file(stream, it);
	}

	*files_size = size;

	if (next != JSON_OBJECT_END)
		errx(1, "Unclosed Gist Files Object");
}

static void
parse_gist(struct json_stream *stream, gcli_gist *out)
{
	enum json_type  next       = JSON_NULL;
	enum json_type  value_type = JSON_NULL;
	const char     *key        = NULL;

	if ((next = json_next(stream)) != JSON_OBJECT)
		errx(1, "Expected Gist Object");

	while ((next = json_next(stream)) == JSON_STRING) {
		size_t len;
		key = json_get_string(stream, &len);

		if (strncmp("owner", key, len) == 0) {
			out->owner = get_user_sv(stream);
		} else if (strncmp("html_url", key, len) == 0) {
			out->url = get_sv(stream);
		} else if (strncmp("id", key, len) == 0) {
			out->id = get_sv(stream);
		} else if (strncmp("created_at", key, len) == 0) {
			out->date = get_sv(stream);
		} else if (strncmp("git_pull_url", key, len) == 0) {
			out->git_pull_url = get_sv(stream);
		} else if (strncmp("description", key, len) == 0) {
			out->description = get_sv(stream);
		} else if (strncmp("files", key, len) == 0) {
			parse_gist_files(stream, &out->files, &out->files_size);
		} else {
			value_type = json_next(stream);

			switch (value_type) {
			case JSON_ARRAY:
				json_skip_until(stream, JSON_ARRAY_END);
				break;
			case JSON_OBJECT:
				json_skip_until(stream, JSON_OBJECT_END);
				break;
			default:
				break;
			}
		}
	}

	if (next != JSON_OBJECT_END)
		errx(1, "Unclosed Gist Object");
}

int
gcli_get_gists(const char *user, int max, gcli_gist **out)
{
	char               *url      = NULL;
	char               *next_url = NULL;
	gcli_fetch_buffer  buffer   = {0};
	struct json_stream  stream   = {0};
	enum   json_type    next     = JSON_NULL;
	int                 size     = 0;

	if (user)
		url = sn_asprintf(
			"%s/users/%s/gists",
			github_get_apibase(),
			user);
	else
		url = sn_asprintf("%s/gists", github_get_apibase());

	do {
		gcli_fetch(url, &next_url, &buffer);

		json_open_buffer(&stream, buffer.data, buffer.length);
		json_set_streaming(&stream, 1);

		if ((next = json_next(&stream)) != JSON_ARRAY)
			errx(1, "Expected array in response");

		while ((next = json_peek(&stream)) == JSON_OBJECT) {
			*out = realloc(*out, sizeof(gcli_gist) * (size + 1));
			gcli_gist *it = &(*out)[size++];
			parse_gist(&stream, it);

			if (size == max)
				break;
		}

		if ((next = json_next(&stream)) != JSON_ARRAY_END)
			errx(1, "Expected end of array in response");

		json_close(&stream);
		free(buffer.data);
		free(url);
	} while ((url = next_url) && (max == -1 || size < max));

	free(next_url);

	return size;
}

static const char *
human_readable_size(size_t s)
{
	if (s < 1024) {
		return sn_asprintf(
			"%zu B",
			s);
	}

	if (s < 1024 * 1024) {
		return sn_asprintf(
			"%zu KiB",
			s / 1024);
	}

	if (s < 1024 * 1024 * 1024) {
		return sn_asprintf(
			"%zu MiB",
			s / (1024 * 1024));
	}

	return sn_asprintf(
		"%zu GiB",
		s / (1024 * 1024 * 1024));
}

static inline const char *
language_fmt(const char *it)
{
	if (it)
		return it;
	else
		return "Unknown";
}

static void
print_gist_file(gcli_gist_file *file)
{
	printf("      ??? %-15.15s  %-8.8s  %-s\n",
		   language_fmt(file->language.data),
		   human_readable_size(file->size),
		   file->filename.data);
}

static void
print_gist(gcli_gist *gist)
{
	printf("   ID : %s"SV_FMT"%s\n"
		   "OWNER : %s"SV_FMT"%s\n"
		   "DESCR : "SV_FMT"\n"
		   " DATE : "SV_FMT"\n"
		   "  URL : "SV_FMT"\n"
		   " PULL : "SV_FMT"\n",
		   gcli_setcolor(GCLI_COLOR_YELLOW), SV_ARGS(gist->id), gcli_resetcolor(),
		   gcli_setbold(), SV_ARGS(gist->owner), gcli_resetbold(),
		   SV_ARGS(gist->description),
		   SV_ARGS(gist->date),
		   SV_ARGS(gist->url),
		   SV_ARGS(gist->git_pull_url));
	printf("FILES : %-15.15s  %-8.8s  %-s\n",
		   "LANGUAGE", "SIZE", "FILENAME");

	for (size_t i = 0; i < gist->files_size; ++i)
		print_gist_file(&gist->files[i]);
}

void
gcli_print_gists_table(
	enum gcli_output_order  order,
	gcli_gist              *gists,
	int                      gists_size)
{
	if (gists_size == 0) {
		puts("No Gists");
		return;
	}

	/* output in reverse order if the sorted flag was enabled */
	if (order == OUTPUT_ORDER_SORTED) {
		for (int i = gists_size; i > 0; --i) {
			print_gist(&gists[i - 1]);
			putchar('\n');
		}
	} else {
		for (int i = 0; i < gists_size; ++i) {
			print_gist(&gists[i]);
			putchar('\n');
		}
	}
}

gcli_gist *
gcli_get_gist(const char *gist_id)
{
	char               *url    = NULL;
	gcli_fetch_buffer  buffer = {0};
	struct json_stream  stream = {0};
	gcli_gist         *it     = NULL;

	url = sn_asprintf("%s/gists/%s", github_get_apibase(), gist_id);

	gcli_fetch(url, NULL, &buffer);

	json_open_buffer(&stream, buffer.data, buffer.length);
	json_set_streaming(&stream, 1);

	it = calloc(sizeof(gcli_gist), 1);
	parse_gist(&stream, it);

	json_close(&stream);
	free(buffer.data);
	free(url);

	return it;
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

void
gcli_create_gist(gcli_new_gist opts)
{
	char               *url          = NULL;
	char               *post_data    = NULL;
	gcli_fetch_buffer  fetch_buffer = {0};
	sn_sv               read_buffer  = {0};
	sn_sv               content      = {0};

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
	url = sn_asprintf("%s/gists", github_get_apibase());
	post_data = sn_asprintf(
		"{\"description\":\"%s\",\"public\":true,\"files\":"
		"{\"%s\": {\"content\":\""SV_FMT"\"}}}",
		opts.gist_description,
		opts.file_name,
		SV_ARGS(content));

	gcli_fetch_with_method("POST", url, post_data, NULL, &fetch_buffer);
	gcli_print_html_url(fetch_buffer);

	free(read_buffer.data);
	free(fetch_buffer.data);
	free(url);
	free(post_data);
}

void
gcli_delete_gist(const char *gist_id, bool always_yes)
{
	char               *url    = NULL;
	gcli_fetch_buffer  buffer = {0};

	url = sn_asprintf(
		"%s/gists/%s",
		github_get_apibase(),
		gist_id);

	if (!always_yes && !sn_yesno("Are you sure you want to delete this gist?"))
		errx(1, "Aborted by user");

	gcli_fetch_with_method("DELETE", url, NULL, NULL, &buffer);

	free(buffer.data);
	free(url);
}
