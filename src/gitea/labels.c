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

#include <gcli/gitea/config.h>
#include <gcli/gitea/labels.h>
#include <gcli/gitea/repos.h>
#include <gcli/github/labels.h>
#include <gcli/json_gen.h>
#include <gcli/json_util.h>

#include <pdjson/pdjson.h>
#include <sn/sn.h>

/* Recycle the GitHub parser for labels */
#include <templates/github/labels.h>

int
gitea_get_labels(struct gcli_ctx *ctx, struct gcli_path const *const path,
                 int max, struct gcli_label_list *const list)
{
	return github_get_labels(ctx, path, max, list);
}

int
gitea_create_label(struct gcli_ctx *ctx, struct gcli_path const *const path,
                   struct gcli_label *const label)
{
	return github_create_label(ctx, path, label);
}

/* Resolve the label name to an ID. */
static int
gitea_get_label_id(struct gcli_ctx *ctx, struct gcli_path const *const path,
                   char const *const label_name, gcli_id *const id)
{
	struct gcli_label_list list = {0};
	int rc = 0;

	*id = 0;

	rc = gitea_get_labels(ctx, path, -1, &list);
	if (rc < 0)
		return rc;

	/* Search for the id */
	for (size_t i = 0; i < list.labels_size; ++i) {
		if (strcmp(list.labels[i].name, label_name) == 0) {
			*id = list.labels[i].id;
			rc = 0;
			goto done;
		}
	}

	/* not found */
	rc = gcli_error(ctx, "%s: no such label", label_name);

done:
	gcli_free_labels(&list);

	return rc;
}

static int
gitea_label_make_url(struct gcli_ctx *ctx, struct gcli_path const *const path,
                     char **url, char const *const fmt, ...)
{
	char *suffix = NULL;
	int rc = 0;
	va_list vp;

	va_start(vp, fmt);
	suffix = sn_vasprintf(fmt, vp);
	va_end(vp);

	switch (path->kind) {
	case GCLI_PATH_DEFAULT: {
		char *e_owner = NULL, *e_repo = NULL;

		e_owner = gcli_urlencode(path->as_default.owner);
		e_repo  = gcli_urlencode(path->as_default.repo);

		*url = sn_asprintf("%s/repos/%s/%s/labels/%"PRIid"%s",
		                   gcli_get_apibase(ctx), e_owner, e_repo,
		                   path->as_default.id, suffix);

		free(e_owner);
		free(e_repo);
	} break;
	case GCLI_PATH_NAMED: {
		struct gcli_path repo_path = {0};
		char *e_owner, *e_repo;
		gcli_id id = 0;

		/* prepare the path to the repository */
		repo_path.kind = GCLI_PATH_DEFAULT;
		repo_path.as_default.owner = path->as_named.owner;
		repo_path.as_default.repo = path->as_named.repo;

		/* resolve the id */
		rc = gitea_get_label_id(ctx, &repo_path, path->as_named.id, &id);
		if (rc < 0)
			goto done;

		/* now make the actual URL */
		e_owner = gcli_urlencode(path->as_named.owner);
		e_repo = gcli_urlencode(path->as_named.repo);

		*url = sn_asprintf("%s/repos/%s/%s/labels/%"PRIid"%s",
		                   gcli_get_apibase(ctx), e_owner, e_repo, id,
		                   suffix);

		gcli_clear_ptr(&e_owner);
		gcli_clear_ptr(&e_repo);
	} break;
	case GCLI_PATH_URL: {
		*url = sn_asprintf("%s%s", path->as_url, suffix);
	} break;
	default: {
		rc = gcli_error(ctx, "unsupported path kind for Gitea issues");
	} break;
	}

done:
	gcli_clear_ptr(&suffix);

	return rc;
}

int
gitea_delete_label(struct gcli_ctx *ctx, struct gcli_path const *const path)
{
	char *url = NULL;
	int rc = 0;

	/* DELETE /repos/{owner}/{repo}/labels/{} */
	rc = gitea_label_make_url(ctx, path, &url, "");

	if (rc == 0) {
		rc = gcli_fetch_with_method(ctx, "DELETE", url, NULL, NULL, NULL);
	}

	free(url);

	return rc;
}

int
gitea_get_label(struct gcli_ctx *ctx, struct gcli_path const *const path,
                struct gcli_label *const out)
{
	int rc = 0;
	char *url = NULL;
	struct gcli_fetch_buffer buffer = {0};

	rc = gitea_label_make_url(ctx, path, &url, "");
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
gitea_label_update_property(struct gcli_ctx *ctx, struct gcli_path const *const path,
                            char const *const propname, char const *const val)
{
	char *url = NULL, *payload = NULL;
	int rc = 0;
	struct gcli_jsongen gen = {0};

	/* generate URL */
	rc = gitea_label_make_url(ctx, path, &url, "");
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
gitea_label_set_title(struct gcli_ctx *ctx, struct gcli_path const *const path,
                      char const *const new_name)
{
	return gitea_label_update_property(ctx, path, "name", new_name);
}

int
gitea_label_set_description(struct gcli_ctx *ctx,
                            struct gcli_path const *const path,
                            char const *const description)
{
	return gitea_label_update_property(
		ctx, path, "description", description);
}

int
gitea_label_set_colour(struct gcli_ctx *ctx, struct gcli_path const *const path,
                       uint32_t const colour)
{
	char *colour_string = NULL;
	int rc = 0;

	colour_string = sn_asprintf("#%06X", colour & 0xFFFFFF);
	rc = gitea_label_update_property(ctx, path, "color", colour_string);

	gcli_clear_ptr(&colour_string);

	return rc;
}
