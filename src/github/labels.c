/*
 * Copyright 2022-2025 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/github/labels.h>
#include <gcli/github/repos.h>
#include <gcli/json_gen.h>
#include <gcli/json_util.h>

#include <pdjson/pdjson.h>

#include <templates/github/labels.h>

int
github_get_labels(struct gcli_ctx *ctx, struct gcli_path const *const path,
                  int const max, struct gcli_label_list *const out)
{
	char *url = NULL;
	int rc = 0;
	struct gcli_fetch_list_ctx fl = {
		.listp = &out->labels,
		.sizep= &out->labels_size,
		.parse = (parsefn)(parse_github_labels),
		.max = max,
	};

	*out = (struct gcli_label_list) {0};

	rc = github_repo_make_url(ctx, path, &url, "/labels");
	if (rc < 0)
		return rc;

	return gcli_fetch_list(ctx, url, &fl);
}

int
github_create_label(struct gcli_ctx *ctx, struct gcli_path const *const path,
                    struct gcli_label *const label)
{
	char *url = NULL, *payload = NULL, *colour = NULL;
	int rc = 0;
	struct gcli_fetch_buffer buffer = {0};
	struct gcli_jsongen gen = {0};
	struct json_stream stream = {0};

	/* Generate URL: /repos/{owner}/{repo}/labels */
	rc = github_repo_make_url(ctx, path, &url, "/labels");
	if (rc < 0)
		return rc;

	/* Generate payload */
	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "name");
		gcli_jsongen_string(&gen, label->name);

		gcli_jsongen_objmember(&gen, "description");
		gcli_jsongen_string(&gen, label->description);

		colour = sn_asprintf("%06X", label->colour & 0xFFFFFF);

		gcli_jsongen_objmember(&gen, "color");
		gcli_jsongen_string(&gen, colour);

		free(colour);
		colour = NULL;
	}
	gcli_jsongen_end_object(&gen);

	payload = gcli_jsongen_to_string(&gen);
	gcli_jsongen_free(&gen);

	rc = gcli_fetch_with_method(ctx, "POST", url, payload, NULL, &buffer);

	if (rc == 0) {
		json_open_buffer(&stream, buffer.data, buffer.length);
		parse_github_label(ctx, &stream, label);
		json_close(&stream);
	}

	free(url);
	free(payload);
	gcli_fetch_buffer_free(&buffer);

	return rc;
}

static int
github_label_make_url(struct gcli_ctx *ctx, struct gcli_path const *const path,
                      char **url, char const *const fmt, ...)
{
	int rc = 0;
	va_list vp;
	char *suffix = NULL;

	va_start(vp, fmt);
	suffix = sn_vasprintf(fmt, vp);
	va_end(vp);

	/* TODO: add support for ID-based label adressing which in turn
	 * would resolve the id to a name. Yes it's inefficient but it's
	 * what GitHub wants us to do. */
	switch (path->kind) {
	case GCLI_PATH_NAMED: {
		char *e_owner, *e_repo, *e_label;

		e_owner = gcli_urlencode(path->as_named.owner);
		e_repo = gcli_urlencode(path->as_named.repo);
		e_label = gcli_urlencode(path->as_named.id);

		*url = sn_asprintf("%s/repos/%s/%s/labels/%s%s",
		                   gcli_get_apibase(ctx), e_owner, e_repo,
		                   e_label, suffix);

		gcli_clear_ptr(&e_owner);
		gcli_clear_ptr(&e_repo);
		gcli_clear_ptr(&e_label);
	} break;
	case GCLI_PATH_URL: {
		*url = sn_asprintf("%s%s", path->as_url, suffix);
	} break;
	default: {
		rc = gcli_error(ctx, "unsupported path kind for GitHub labels");
	} break;
	}

	gcli_clear_ptr(&suffix);

	return rc;
}

int
github_delete_label(struct gcli_ctx *ctx, struct gcli_path const *const path)
{
	char *url = NULL;
	int rc = 0;

	/* DELETE /repos/{owner}/{repo}/labels/{name} */
	rc = github_label_make_url(ctx, path, &url, "");

	if (rc == 0)
		rc = gcli_fetch_with_method(ctx, "DELETE", url, NULL, NULL, NULL);

	gcli_clear_ptr(&url);

	return rc;
}

int
github_get_label(struct gcli_ctx *ctx, struct gcli_path const *const path,
                 struct gcli_label *const out)
{
	int rc = 0;
	char *url = NULL;
	struct gcli_fetch_buffer buffer = {0};

	rc = github_label_make_url(ctx, path, &url, "");
	if (rc < 0)
		return rc;

	rc = gcli_fetch(ctx, url, NULL, &buffer);
	if (rc == 0) {
		struct json_stream stream = {0};

		json_open_buffer(&stream, buffer.data, buffer.length);
		parse_github_label(ctx, &stream, out);
		json_close(&stream);
	}

	gcli_fetch_buffer_free(&buffer);
	gcli_clear_ptr(&url);

	return rc;
}

static int
github_label_update_property(struct gcli_ctx *ctx,
                             struct gcli_path const *const path,
                             char const *const propname,
                             char const *const val)
{
	char *url = NULL, *payload = NULL;
	int rc = 0;
	struct gcli_jsongen gen = {0};

	/* generate URL */
	rc = github_label_make_url(ctx, path, &url, "");
	if (rc < 0)
		return rc;

	/* generate payload */
	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, propname);
		gcli_jsongen_string(&gen, val);
	}
	gcli_jsongen_end_object(&gen);

	payload = gcli_jsongen_to_string(&gen);
	gcli_jsongen_free(&gen);

	/* perform request */
	rc = gcli_fetch_with_method(ctx, "PATCH", url, payload, NULL, NULL);

	gcli_clear_ptr(&url);
	gcli_clear_ptr(&payload);

	return rc;
}

int
github_label_set_title(struct gcli_ctx *ctx, struct gcli_path const *const path,
                       char const *const new_name)
{
	return github_label_update_property(ctx, path, "new_name", new_name);
}

int
github_label_set_description(struct gcli_ctx *ctx,
                             struct gcli_path const *const path,
                             char const *const new_description)
{
	return github_label_update_property(
		ctx, path, "description", new_description);
}

int
github_label_set_colour(struct gcli_ctx *ctx,
                        struct gcli_path const *const path,
                        uint32_t const colour)
{
	char *colour_string = NULL;
	int rc = 0;

	colour_string = sn_asprintf("%06X", colour & 0xFFFFFF);
	rc = github_label_update_property(ctx, path, "color", colour_string);

	gcli_clear_ptr(&colour_string);

	return rc;
}
