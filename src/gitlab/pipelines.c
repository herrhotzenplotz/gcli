/*
 * Copyright 2022 Nico Sonack <nsonack@herrhotzenplotz.de>
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
#include <gcli/gitlab/pipelines.h>
#include <gcli/json_util.h>
#include <gcli/pulls.h>

#include <pdjson/pdjson.h>
#include <sn/sn.h>

#include <assert.h>

#include <templates/gitlab/pipelines.h>

static int
fetch_pipelines(gcli_ctx *ctx, char *url, int const max,
                gitlab_pipeline_list *const list)
{
	gcli_fetch_list_ctx fl = {
		.listp = &list->pipelines,
		.sizep = &list->pipelines_size,
		.max = max,
		.parse = (parsefn)(parse_gitlab_pipelines),
	};

	return gcli_fetch_list(ctx, url, &fl);
}

int
gitlab_get_pipelines(gcli_ctx *ctx, char const *owner, char const *repo,
                     int const max, gitlab_pipeline_list *const list)
{
	char *url = NULL;

	url = sn_asprintf("%s/projects/%s%%2F%s/pipelines",
	                  gitlab_get_apibase(ctx), owner, repo);

	return fetch_pipelines(ctx, url, max, list);
}

int
gitlab_get_mr_pipelines(gcli_ctx *ctx, char const *owner, char const *repo,
                        int const mr_id, gitlab_pipeline_list *const list)
{
	char *url = NULL;

	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests/%d/pipelines",
	                  gitlab_get_apibase(ctx), owner, repo, mr_id);

	/* fetch everything */
	return fetch_pipelines(ctx, url, -1, list);
}

void
gitlab_free_pipelines(gitlab_pipeline_list *const list)
{
	for (size_t i = 0; i < list->pipelines_size; ++i) {
		free(list->pipelines[i].status);
		free(list->pipelines[i].created_at);
		free(list->pipelines[i].updated_at);
		free(list->pipelines[i].ref);
		free(list->pipelines[i].sha);
		free(list->pipelines[i].source);
	}
	free(list->pipelines);

	list->pipelines = NULL;
	list->pipelines_size = 0;
}

void
gitlab_pipeline_jobs(gcli_ctx *ctx, char const *owner, char const *repo,
                     long const id, int const count)
{
	gitlab_job_list jobs = {0};
	int rc;

	rc = gitlab_get_pipeline_jobs(ctx, owner, repo, id, count, &jobs);
	if (rc < 0)
		errx(1, "error: failed to get jobs");

	gitlab_print_jobs(ctx, &jobs);
	gitlab_free_jobs(&jobs);
}

int
gitlab_get_pipeline_jobs(gcli_ctx *ctx, char const *owner, char const *repo,
                         long const pipeline, int const max,
                         gitlab_job_list *const out)
{
	char *url = NULL;
	gcli_fetch_list_ctx fl = {
		.listp = &out->jobs,
		.sizep = &out->jobs_size,
		.max = max,
		.parse = (parsefn)(parse_gitlab_jobs),
	};

	url = sn_asprintf("%s/projects/%s%%2F%s/pipelines/%ld/jobs",
	                  gitlab_get_apibase(ctx), owner, repo, pipeline);

	return gcli_fetch_list(ctx, url, &fl);
}

void
gitlab_print_jobs(gcli_ctx *ctx, gitlab_job_list const *const list)
{
	gcli_tbl table;
	gcli_tblcoldef cols[] = {
		{ .name = "ID",         .type = GCLI_TBLCOLTYPE_LONG,   .flags = GCLI_TBLCOL_JUSTIFYR },
		{ .name = "NAME",       .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "STATUS",     .type = GCLI_TBLCOLTYPE_STRING, .flags = GCLI_TBLCOL_STATECOLOURED },
		{ .name = "STARTED",    .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "FINISHED",   .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "RUNNERDESC", .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "REF",        .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
	};

	(void) ctx;

	if (!list->jobs_size) {
		printf("No jobs\n");
		return;
	}

	table = gcli_tbl_begin(cols, ARRAY_SIZE(cols));
	if (!table)
		errx(1, "error: could not initialize table");

	for (size_t i = 0; i < list->jobs_size; ++i) {
		gcli_tbl_add_row(table,
		                 list->jobs[i].id,
		                 list->jobs[i].name,
		                 list->jobs[i].status,
		                 list->jobs[i].started_at,
		                 list->jobs[i].finished_at,
		                 list->jobs[i].runner_description,
		                 list->jobs[i].ref);
	}

	gcli_tbl_end(table);
}

void
gitlab_free_job(gitlab_job *const job)
{
	free(job->status);
	free(job->stage);
	free(job->name);
	free(job->ref);
	free(job->created_at);
	free(job->started_at);
	free(job->finished_at);
	free(job->runner_name);
	free(job->runner_description);
}

void
gitlab_free_jobs(gitlab_job_list *list)
{
	for (size_t i = 0; i < list->jobs_size; ++i)
		gitlab_free_job(&list->jobs[i]);

	free(list->jobs);

	list->jobs = NULL;
	list->jobs_size = 0;
}

int
gitlab_job_get_log(gcli_ctx *ctx, char const *owner, char const *repo,
                   long const job_id)
{
	gcli_fetch_buffer buffer = {0};
	char *url = NULL;
	int rc = 0;

	url = sn_asprintf("%s/projects/%s%%2F%s/jobs/%ld/trace",
	                  gitlab_get_apibase(ctx), owner, repo, job_id);

	rc = gcli_fetch(ctx, url, NULL, &buffer);
	if (rc == 0) {
		fwrite(buffer.data, buffer.length, 1, stdout);
	}

	free(buffer.data);
	free(url);

	return rc;
}

static int
gitlab_get_job(gcli_ctx *ctx, char const *owner, char const *repo,
               long const jid, gitlab_job *const out)
{
	gcli_fetch_buffer buffer = {0};
	char *url = NULL;
	int rc = 0;

	url = sn_asprintf("%s/projects/%s%%2F%s/jobs/%ld",
	                  gitlab_get_apibase(ctx), owner, repo, jid);

	rc = gcli_fetch(ctx, url, NULL, &buffer);
	if (rc == 0) {
		struct json_stream  stream = {0};

		json_open_buffer(&stream, buffer.data, buffer.length);
		json_set_streaming(&stream, 1);
		parse_gitlab_job(ctx, &stream, out);
		json_close(&stream);
	}

	free(buffer.data);
	free(url);

	return rc;
}

static void
gitlab_print_job_status(gcli_ctx *ctx, gitlab_job const *const job)
{
	gcli_dict printer;

	(void) ctx;

	printer = gcli_dict_begin();

	gcli_dict_add(printer,        "ID", 0, 0, "%ld", job->id);
	gcli_dict_add_string(printer, "STATUS", GCLI_TBLCOL_STATECOLOURED, 0, job->status);
	gcli_dict_add_string(printer, "STAGE", 0, 0, job->stage);
	gcli_dict_add_string(printer, "NAME", GCLI_TBLCOL_BOLD, 0, job->name);
	gcli_dict_add_string(printer, "REF", GCLI_TBLCOL_COLOUREXPL, GCLI_COLOR_YELLOW, job->ref);
	gcli_dict_add_string(printer, "CREATED", 0, 0, job->created_at);
	gcli_dict_add_string(printer, "STARTED", 0, 0, job->started_at);
	gcli_dict_add_string(printer, "FINISHED", 0, 0, job->finished_at);
	gcli_dict_add(printer,        "DURATION", 0, 0, "%-.2lfs", job->duration);
	gcli_dict_add_string(printer, "RUNNER NAME", 0, 0, job->runner_name);
	gcli_dict_add_string(printer, "RUNNER DESCR", 0, 0, job->runner_description);

	gcli_dict_end(printer);
}

int
gitlab_job_status(gcli_ctx *ctx, char const *owner, char const *repo,
                  long const jid)
{
	gitlab_job job = {0};
	int rc = 0;

	rc = gitlab_get_job(ctx, owner, repo, jid, &job);
	if (rc == 0)
		gitlab_print_job_status(ctx, &job);

	gitlab_free_job(&job);

	return rc;
}

int
gitlab_job_cancel(gcli_ctx *ctx, char const *owner, char const *repo,
                  long const jid)
{
	char *url = NULL;
	int rc = 0;

	url = sn_asprintf("%s/projects/%s%%2F%s/jobs/%ld/cancel",
	                  gitlab_get_apibase(ctx), owner, repo, jid);
	rc = gcli_fetch_with_method(ctx, "POST", url, NULL, NULL, NULL);

	free(url);

	return rc;
}

int
gitlab_job_retry(gcli_ctx *ctx, char const *owner, char const *repo,
                 long const jid)
{
	int rc = 0;
	char *url = NULL;

	url = sn_asprintf("%s/projects/%s%%2F%s/jobs/%ld/retry",
	                  gitlab_get_apibase(ctx), owner, repo, jid);
	rc = gcli_fetch_with_method(ctx, "POST", url, NULL, NULL, NULL);

	free(url);

	return rc;
}

int
gitlab_mr_pipelines(gcli_ctx *ctx, char const *owner, char const *repo,
                    int const mr_id)
{
	gitlab_pipeline_list list = {0};
	int rc = 0;

	rc = gitlab_get_mr_pipelines(ctx, owner, repo, mr_id, &list);
	if (rc == 0)
		gitlab_print_pipelines(&list);

	gitlab_free_pipelines(&list);

	return rc;
}

int
gitlab_job_download_artifacts(gcli_ctx *ctx, char const *owner,
                              char const *repo, long const jid,
                              char const *const outfile)
{
	char *url;
	char *e_owner, *e_repo;
	FILE *f;
	int rc = 0;

	f = fopen(outfile, "wb");
	if (f == NULL)
		return -1;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/jobs/%ld/artifacts",
	                  gitlab_get_apibase(ctx),
	                  e_owner, e_repo, jid);

	rc = gcli_curl(ctx, f, url, "application/zip");

	fclose(f);
	free(url);
	free(e_owner);
	free(e_repo);

	return rc;
}
