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

#include <gcli/cmd/colour.h>
#include <gcli/cmd/pipelines.h>
#include <gcli/cmd/table.h>

#include <gcli/gitlab/config.h>
#include <gcli/gitlab/merge_requests.h>
#include <gcli/gitlab/pipelines.h>
#include <gcli/gitlab/repos.h>
#include <gcli/json_util.h>
#include <gcli/pulls.h>
#include <gcli/url.h>

#include <pdjson.h>

#include <assert.h>

#include <templates/gitlab/pipelines.h>

static int
fetch_pipelines(struct gcli_ctx *ctx, char *url, int const max,
                struct gitlab_pipeline_list *const list)
{
	struct gcli_fetch_list_ctx fl = {
		.listp = &list->pipelines,
		.sizep = &list->pipelines_size,
		.max = max,
		.parse = (parsefn)(parse_gitlab_pipelines),
	};

	return gcli_fetch_list(ctx, url, &fl);
}

int
gitlab_get_pipelines(struct gcli_ctx *ctx, struct gcli_path const *const path,
                     struct gitlab_pipelines_fetch_details const *details,
                     struct gitlab_pipeline_list *const list)
{
	char *url = NULL, *args = NULL;
	int rc = 0, max = -1;

	if (details) {
		if (details->ref)
			gcli_url_options_append(&args, "ref", details->ref);

		max = details->max;
	}

	rc = gitlab_repo_make_url(ctx, path, &url, "/pipelines%s",
	                          args ? args : "");
	if (rc < 0)
		return rc;

	gcli_clear_ptr(&args);

	return fetch_pipelines(ctx, url, max, list);
}

static int
gitlab_pipeline_make_url(struct gcli_ctx *ctx,
                         struct gcli_path const *const path,
                         char **url,
                         char const *const suffix_fmt, ...)
{
	char *suffix = NULL;
	int rc = 0;
	va_list vp;

	va_start(vp, suffix_fmt);
	suffix = gcli_vasprintf(suffix_fmt, vp);
	va_end(vp);

	switch (path->kind) {
	case GCLI_PATH_DEFAULT: {
		char *e_owner, *e_repo = NULL;

		e_owner = gcli_urlencode(path->as_default.owner);
		e_repo = gcli_urlencode(path->as_default.repo);

		*url = gcli_asprintf("%s/projects/%s%%2F%s/pipelines/%"PRIid"%s",
		                     gcli_get_apibase(ctx), e_owner, e_repo,
		                     path->as_default.id,
		                     suffix);

		gcli_clear_ptr(&e_owner);
		gcli_clear_ptr(&e_repo);
	} break;
	case GCLI_PATH_URL: {
		*url = gcli_asprintf("%s%s", path->as_url, suffix);
	} break;
	default: {
		rc = gcli_error(ctx, "unsupported path type for gitlab pipelines");
	} break;
	}

	gcli_clear_ptr(&suffix);

	return rc;
}

int
gitlab_get_pipeline(struct gcli_ctx *ctx,
                    struct gcli_path const *const pipeline_path,
                    struct gitlab_pipeline *out)
{
	char *url = NULL;
	int rc = 0;
	struct gcli_fetch_buffer buffer = {0};
	struct json_stream stream = {0};

	rc = gitlab_pipeline_make_url(ctx, pipeline_path, &url, "");
	if (rc < 0)
		return rc;

	rc = gcli_fetch(ctx, url, NULL, &buffer);
	if (rc == 0) {
		json_open_buffer(&stream, buffer.data, buffer.length);

		rc = parse_gitlab_pipeline(ctx, &stream, out);

		json_close(&stream);
		gcli_fetch_buffer_free(&buffer);
	}

	gcli_clear_ptr(&url);

	return rc;
}

int
gitlab_get_mr_pipelines(struct gcli_ctx *ctx,
                        struct gcli_path const *const path,
                        struct gitlab_pipeline_list *const list)
{
	char *url = NULL;
	int rc = 0;

	rc = gitlab_mr_make_url(ctx, path, &url, "/pipelines");
	if (rc < 0)
		return rc;

	/* fetch everything */
	return fetch_pipelines(ctx, url, -1, list);
}

void
gitlab_pipeline_free(struct gitlab_pipeline *pipeline)
{
	gcli_clear_ptr(&pipeline->status);
	gcli_clear_ptr(&pipeline->ref);
	gcli_clear_ptr(&pipeline->sha);
	gcli_clear_ptr(&pipeline->source);
	gcli_clear_ptr(&pipeline->web_url);
}

void
gitlab_pipelines_free(struct gitlab_pipeline_list *const list)
{
	for (size_t i = 0; i < list->pipelines_size; ++i) {
		gitlab_pipeline_free(&list->pipelines[i]);
	}

	gcli_clear_ptr(&list->pipelines);
	list->pipelines_size = 0;
}

int
gitlab_get_pipeline_jobs(struct gcli_ctx *ctx,
                         struct gcli_path const *const pipeline_path,
                         int const max, struct gitlab_job_list *const out)
{
	char *url = NULL;
	int rc = 0;

	struct gcli_fetch_list_ctx fl = {
		.listp = &out->jobs,
		.sizep = &out->jobs_size,
		.max = max,
		.parse = (parsefn)(parse_gitlab_jobs),
	};

	rc = gitlab_pipeline_make_url(ctx, pipeline_path, &url, "/jobs");
	if (rc < 0)
		return rc;

	rc = gcli_fetch_list(ctx, url, &fl);
	if (rc < 0)
		return rc;

	/* Fetch trigger jobs as well, useful when trying to re-run triggered
	 * pipelines. */
	rc = gitlab_pipeline_make_url(ctx, pipeline_path, &url, "/bridges");
	if (rc < 0)
		return rc;

	return gcli_fetch_list(ctx, url, &fl);
}

int
gitlab_get_pipeline_children(struct gcli_ctx *ctx,
                             struct gcli_path const *const pipeline_path,
                             int count,
                             struct gitlab_pipeline_list *out)
{
	char *url = NULL;
	int rc = 0;
	struct gcli_fetch_list_ctx fl = {
		.listp = &out->pipelines,
		.sizep = &out->pipelines_size,
		.max = count,
		.parse = (parsefn)(parse_gitlab_pipeline_children),
	};

	rc = gitlab_pipeline_make_url(ctx, pipeline_path, &url, "/bridges");
	if (rc < 0)
		return rc;

	return gcli_fetch_list(ctx, url, &fl);
}

void
gitlab_free_job(struct gitlab_job *const job)
{
	gcli_clear_ptr(&job->status);
	gcli_clear_ptr(&job->stage);
	gcli_clear_ptr(&job->name);
	gcli_clear_ptr(&job->ref);
	gcli_clear_ptr(&job->runner_name);
	gcli_clear_ptr(&job->runner_description);
	gcli_clear_ptr(&job->web_url);
}

void
gitlab_free_jobs(struct gitlab_job_list *list)
{
	for (size_t i = 0; i < list->jobs_size; ++i)
		gitlab_free_job(&list->jobs[i]);

	gcli_clear_ptr(&list->jobs);
	list->jobs_size = 0;
}

static int
gitlab_job_make_url(struct gcli_ctx *ctx,
                    struct gcli_path const *const path,
                    char **url,
                    char const *const suffix_fmt, ...)
{
	char *suffix = NULL;
	int rc = 0;
	va_list vp;

	va_start(vp, suffix_fmt);
	suffix = gcli_vasprintf(suffix_fmt, vp);
	va_end(vp);

	switch (path->kind) {
	case GCLI_PATH_DEFAULT: {
		char *e_owner, *e_repo = NULL;

		e_owner = gcli_urlencode(path->as_default.owner);
		e_repo = gcli_urlencode(path->as_default.repo);

		*url = gcli_asprintf("%s/projects/%s%%2F%s/jobs/%"PRIid"%s",
		                     gcli_get_apibase(ctx), e_owner, e_repo,
		                     path->as_default.id,
		                     suffix);

		gcli_clear_ptr(&e_owner);
		gcli_clear_ptr(&e_repo);
	} break;
	case GCLI_PATH_URL: {
		*url = gcli_asprintf("%s%s", path->as_url, suffix);
	} break;
	default: {
		rc = gcli_error(ctx, "unsupported path type for gitlab jobs");
	} break;
	}

	gcli_clear_ptr(&suffix);

	return rc;
}

int
gitlab_job_get_log(struct gcli_ctx *ctx, struct gcli_path const *const job_path,
                   FILE *stream)
{
	char *url = NULL;
	int rc = 0;

	rc = gitlab_job_make_url(ctx, job_path, &url, "/trace");
	if (rc < 0)
		return rc;

	rc = gcli_curl(ctx, stream, url, NULL);

	gcli_clear_ptr(&url);

	return rc;
}

int
gitlab_get_job(struct gcli_ctx *ctx, struct gcli_path const *const job_path,
               struct gitlab_job *const out)
{
	struct gcli_fetch_buffer buffer = {0};
	char *url = NULL;
	int rc = 0;

	rc = gitlab_job_make_url(ctx, job_path, &url, "");
	if (rc < 0)
		return rc;

	rc = gcli_fetch(ctx, url, NULL, &buffer);
	if (rc == 0) {
		struct json_stream  stream = {0};

		json_open_buffer(&stream, buffer.data, buffer.length);
		json_set_streaming(&stream, 1);
		parse_gitlab_job(ctx, &stream, out);
		json_close(&stream);
	}

	gcli_fetch_buffer_free(&buffer);
	gcli_clear_ptr(&url);

	return rc;
}

int
gitlab_job_cancel(struct gcli_ctx *ctx, struct gcli_path const *const path)
{
	char *url = NULL;
	int rc = 0;

	rc = gitlab_job_make_url(ctx, path, &url, "/cancel");
	if (rc < 0)
		return rc;

	rc = gcli_fetch_with_method(ctx, "POST", url, NULL, NULL, NULL);

	gcli_clear_ptr(&url);

	return rc;
}

int
gitlab_job_retry(struct gcli_ctx *ctx, struct gcli_path const *const path)
{
	int rc = 0;
	char *url = NULL;

	rc = gitlab_job_make_url(ctx, path, &url, "/retry");
	if (rc < 0)
		return rc;

	rc = gcli_fetch_with_method(ctx, "POST", url, NULL, NULL, NULL);

	gcli_clear_ptr(&url);

	return rc;
}

int
gitlab_job_download_artifacts(struct gcli_ctx *ctx,
                              struct gcli_path const *const job_path,
                              char const *const outfile)
{
	FILE *f = NULL;
	char *url = NULL;
	int rc = 0;

	f = fopen(outfile, "wb");
	if (f == NULL)
		rc = gcli_error(ctx, "failed to open output file %s", outfile);

	if (rc == 0)
		rc = gitlab_job_make_url(ctx, job_path, &url, "/artifacts");

	if (rc == 0)
		rc = gcli_curl(ctx, f, url, "application/zip");

	if (f)
		fclose(f);

	gcli_clear_ptr(&url);

	return rc;
}
