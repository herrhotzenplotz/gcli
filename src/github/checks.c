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

#include <assert.h>

#include <gcli/cmd/colour.h>
#include <gcli/cmd/table.h>
#include <gcli/curl.h>
#include <gcli/github/checks.h>
#include <gcli/github/path.h>
#include <gcli/github/repos.h>
#include <gcli/json_util.h>

#include <templates/github/checks.h>

#include <pdjson.h>

static int
github_checks_make_url(struct gcli_ctx *ctx,
                       char const *const type,
                       struct gcli_path const *const path,
                       char **url,
                       char const *const fmt, va_list *vp)
{
	int rc = 0;
	char *suffix = NULL;

	suffix = gcli_vasprintf(fmt, *vp);

	switch (path->kind) {
	case GCLI_PATH_DEFAULT: {
		char *e_owner, *e_repo;

		e_owner = gcli_urlencode(path->as_default.owner);
		e_repo = gcli_urlencode(path->as_default.repo);

		*url = gcli_asprintf("%s/repos/%s/%s/%s/%"PRIid"%s",
		                     gcli_get_apibase(ctx), e_owner, e_repo,
		                     type, path->as_default.id, suffix);

		gcli_clear_ptr(&e_owner);
		gcli_clear_ptr(&e_repo);
	} break;
	case GCLI_PATH_URL: {
		*url = gcli_asprintf("%s%s", path->as_url, suffix);
	} break;
	default: {
		rc = gcli_error(ctx, "unsupported path type for github checksuite");
	} break;
	}

	gcli_clear_ptr(&suffix);

	return rc;
}

int
github_checksuite_make_url(struct gcli_ctx *ctx,
                           struct gcli_path const *const path,
                           char **url, char const *const fmt, ...)
{
	int rc = 0;
	va_list vp;

	va_start(vp, fmt);
	rc = github_checks_make_url(ctx, "check-suites", path, url, fmt, &vp);
	va_end(vp);

	return rc;
}

int
github_checkrun_make_url(struct gcli_ctx *ctx,
                         struct gcli_path const *const path,
                         char **url, char const *const fmt, ...)
{
	int rc = 0;
	va_list vp;

	va_start(vp, fmt);
	rc = github_checks_make_url(ctx, "check-runs", path, url, fmt, &vp);
	va_end(vp);

	return rc;
}

int
github_get_check_suites(struct gcli_ctx *ctx,
                        struct gcli_path const *const path,
                        struct gcli_pipelines_fetch_details const *details,
                        struct gcli_pipeline_list *const out)
{
	struct gcli_fetch_list_ctx flctx = {0};
	struct gcli_path norm_path = {0};
	char const *ref = "HEAD";
	char *url = NULL;
	int rc = 0;

	assert(out);

	/* we must normalise these paths to default paths. otherwise we'd get
	 * bugs when a URL path to some item is passed in. */
	rc = github_path_normalise(ctx, path, &norm_path);
	if (rc < 0)
		return rc;

	if (details->ref)
		ref = details->ref;

	rc = github_repo_make_url(ctx, path, &url, "/commits/%s/check-suites", ref);
	if (rc < 0)
		return rc;

	flctx.listp = out;
	flctx.sizep = &out->pipelines_size;
	flctx.max = details->max;
	flctx.flags = GCLI_FL_ARRAYPARSER;
	flctx.arrparse = (arrparsefn)parse_github_checksuites;

	rc = gcli_fetch_list(ctx, url, &flctx);

	gcli_path_free(&norm_path);

	return rc;
}

int
github_get_check_runs(struct gcli_ctx *ctx,
                      struct gcli_path const *pipeline_path,
                      int max,
                      struct gcli_job_list *out)
{
	char *url = NULL;
	int rc = 0;
	struct gcli_fetch_list_ctx flctx = {0};
	struct gcli_path norm_path = {0};

	assert(out);

	rc = github_path_normalise(ctx, pipeline_path, &norm_path);
	if (rc < 0)
		return rc;

	rc = github_checksuite_make_url(ctx, &norm_path, &url, "/check-runs");
	if (rc < 0)
		return rc;

	flctx.listp = out;
	flctx.sizep = &out->jobs_size;
	flctx.max = max;
	flctx.flags = GCLI_FL_ARRAYPARSER;
	flctx.arrparse = (arrparsefn)parse_github_check_runs;

	rc = gcli_fetch_list(ctx, url, &flctx);

	gcli_path_free(&norm_path);

	return rc;
}

int
github_get_check_suite(struct gcli_ctx *ctx,
                       struct gcli_path const *pipeline_path,
                       struct gcli_pipeline *out)
{
	char *url = NULL;
	int rc = 0;
	struct gcli_fetch_buffer buffer = {0};
	struct gcli_path norm_path = {0};
	struct json_stream stream = {0};

	rc = github_path_normalise(ctx, pipeline_path, &norm_path);
	if (rc < 0)
		return rc;

	rc = github_checksuite_make_url(ctx, &norm_path, &url, "");
	gcli_path_free(&norm_path);

	if (rc < 0)
		return rc;

	rc = gcli_fetch(ctx, url, NULL, &buffer);
	if (rc == 0) {
		json_open_buffer(&stream, buffer.data, buffer.length);
		rc = parse_github_checksuite(ctx, &stream, out);
		json_close(&stream);
	}

	gcli_fetch_buffer_free(&buffer);
	gcli_clear_ptr(&url);

	return rc;
}
