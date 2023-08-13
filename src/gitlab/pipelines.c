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

#include <gcli/colour.h>
#include <gcli/gitlab/config.h>
#include <gcli/gitlab/pipelines.h>
#include <gcli/json_util.h>
#include <gcli/table.h>
#include <pdjson/pdjson.h>
#include <sn/sn.h>

#include <assert.h>

#include <templates/gitlab/pipelines.h>

static int
fetch_pipelines(char *url, int const max, gitlab_pipeline_list *const list)
{
	char               *next_url = NULL;
	gcli_fetch_buffer   buffer   = {0};
	struct json_stream  stream   = {0};

	*list = (gitlab_pipeline_list) {0};

	do {
		gcli_fetch(url, &next_url, &buffer);

		json_open_buffer(&stream, buffer.data, buffer.length);

		parse_gitlab_pipelines(&stream, &list->pipelines, &list->pipelines_size);

		json_close(&stream);
		free(buffer.data);
		free(url);
	} while ((url = next_url) && (max == -1 || (int)list->pipelines_size < max));

	return 0;
}

int
gitlab_get_pipelines(char const *owner,
                     char const *repo,
                     int const max,
                     gitlab_pipeline_list *const list)
{
	char *url = NULL;

	url = sn_asprintf("%s/projects/%s%%2F%s/pipelines",
	                  gitlab_get_apibase(), owner, repo);

	return fetch_pipelines(url, max, list);
}

static int
gitlab_get_mr_pipelines(char const *owner, char const *repo, int const mr_id,
                        gitlab_pipeline_list *const list)
{
	char *url = NULL;

	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests/%d/pipelines",
	                  gitlab_get_apibase(), owner, repo, mr_id);

	/* fetch everything */
	return fetch_pipelines(url, -1, list);
}

void
gitlab_print_pipelines(gitlab_pipeline_list const *const list)
{
	gcli_tbl table;
	gcli_tblcoldef cols[] = {
		{ .name = "ID",      .type = GCLI_TBLCOLTYPE_INT,    .flags = GCLI_TBLCOL_JUSTIFYR },
		{ .name = "STATUS",  .type = GCLI_TBLCOLTYPE_STRING, .flags = GCLI_TBLCOL_STATECOLOURED },
		{ .name = "CREATED", .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "UPDATED", .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "REF",     .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
	};

	if (!list->pipelines_size) {
		printf("No pipelines\n");
		return;
	}

	table = gcli_tbl_begin(cols, ARRAY_SIZE(cols));
	if (!table)
		errx(1, "error: could not init table");

	for (size_t i = 0; i < list->pipelines_size; ++i) {
		gcli_tbl_add_row(table,
		                 (int)(list->pipelines[i].id),
		                 list->pipelines[i].status,
		                 list->pipelines[i].created_at,
		                 list->pipelines[i].updated_at,
		                 list->pipelines[i].ref);
	}

	gcli_tbl_end(table);
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
gitlab_pipelines(char const *owner, char const *repo, int const count)
{
	gitlab_pipeline_list pipelines = {0};

	gitlab_get_pipelines(owner, repo, count, &pipelines);
	gitlab_print_pipelines(&pipelines);
	gitlab_free_pipelines(&pipelines);
}

void
gitlab_pipeline_jobs(char const *owner,
                     char const *repo,
                     long const id,
                     int const count)
{
	gitlab_job *jobs = NULL;
	int const jobs_size = gitlab_get_pipeline_jobs(owner, repo, id, count, &jobs);
	gitlab_print_jobs(jobs, jobs_size);
	gitlab_free_jobs(jobs, jobs_size);
}

int
gitlab_get_pipeline_jobs(char const *owner,
                         char const *repo,
                         long const pipeline,
                         int const max,
                         gitlab_job **const out)
{
	char   *url      = NULL;
	char   *next_url = NULL;
	size_t  out_size = 0;

	url = sn_asprintf("%s/projects/%s%%2F%s/pipelines/%ld/jobs",
	                  gitlab_get_apibase(), owner, repo, pipeline);

	do {
		gcli_fetch_buffer  buffer = {0};
		struct json_stream stream = {0};

		gcli_fetch(url, &next_url, &buffer);
		json_open_buffer(&stream, buffer.data, buffer.length);

		parse_gitlab_jobs(&stream, out, &out_size);

		free(url);
	} while ((url = next_url) && (((int)out_size < max) || (max == -1)));

	return (int)out_size;
}

void
gitlab_print_jobs(gitlab_job const *const jobs, int const jobs_size)
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

	if (!jobs_size) {
		printf("No jobs\n");
		return;
	}

	table = gcli_tbl_begin(cols, ARRAY_SIZE(cols));
	if (!table)
		errx(1, "error: could not initialize table");

	for (int i = 0; i < jobs_size; ++i) {
		gcli_tbl_add_row(table,
		                 jobs[i].id,
		                 jobs[i].name,
		                 jobs[i].status,
		                 jobs[i].started_at,
		                 jobs[i].finished_at,
		                 jobs[i].runner_description,
		                 jobs[i].ref);
	}

	gcli_tbl_end(table);
}

static void
gitlab_free_job_data(gitlab_job *const job)
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
gitlab_free_jobs(gitlab_job *jobs, int const jobs_size)
{
	for (int i = 0; i < jobs_size; ++i) {
		gitlab_free_job_data(&jobs[i]);
	}
	free(jobs);
}

int
gitlab_job_get_log(char const *owner, char const *repo, long const job_id)
{
	gcli_fetch_buffer buffer = {0};
	char *url = NULL;
	int rc = 0;

	url = sn_asprintf("%s/projects/%s%%2F%s/jobs/%ld/trace",
	                  gitlab_get_apibase(), owner, repo, job_id);

	rc = gcli_fetch(url, NULL, &buffer);
	if (rc == 0) {
		fwrite(buffer.data, buffer.length, 1, stdout);
	}

	free(buffer.data);
	free(url);

	return rc;
}

static int
gitlab_get_job(char const *owner,
               char const *repo,
               long const jid,
               gitlab_job *const out)
{
	gcli_fetch_buffer buffer = {0};
	char *url = NULL;
	int rc = 0;

	url = sn_asprintf("%s/projects/%s%%2F%s/jobs/%ld",
	                  gitlab_get_apibase(), owner, repo, jid);

	rc = gcli_fetch(url, NULL, &buffer);
	if (rc == 0) {
		struct json_stream  stream = {0};

		json_open_buffer(&stream, buffer.data, buffer.length);
		json_set_streaming(&stream, 1);
		parse_gitlab_job(&stream, out);
		json_close(&stream);
	}

	free(buffer.data);
	free(url);

	return rc;
}

static void
gitlab_print_job_status(gitlab_job const *const job)
{
	gcli_dict printer;

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
gitlab_job_status(char const *owner, char const *repo, long const jid)
{
	gitlab_job job = {0};
	int rc = 0;

	rc = gitlab_get_job(owner, repo, jid, &job);
	if (rc == 0)
		gitlab_print_job_status(&job);

	gitlab_free_job_data(&job);

	return rc;
}

int
gitlab_job_cancel(char const *owner, char const *repo, long const jid)
{
	char *url = NULL;
	int rc = 0;

	url = sn_asprintf("%s/projects/%s%%2F%s/jobs/%ld/cancel",
	                  gitlab_get_apibase(), owner, repo, jid);
	rc = gcli_fetch_with_method("POST", url, NULL, NULL, NULL);

	free(url);

	return rc;
}

int
gitlab_job_retry(char const *owner, char const *repo, long const jid)
{
	int   rc  = 0;
	char *url = NULL;

	url = sn_asprintf("%s/projects/%s%%2F%s/jobs/%ld/retry",
	                  gitlab_get_apibase(), owner, repo, jid);
	rc = gcli_fetch_with_method("POST", url, NULL, NULL, NULL);

	free(url);

	return rc;
}

int
gitlab_mr_pipelines(char const *owner, char const *repo, int const mr_id)
{
	gitlab_pipeline_list list = {0};
	int rc = 0;

	rc = gitlab_get_mr_pipelines(owner, repo, mr_id, &list);
	if (rc == 0)
		gitlab_print_pipelines(&list);

	gitlab_free_pipelines(&list);

	return rc;
}

int
gitlab_job_download_artifacts(char const *owner, char const *repo,
                              long const jid, char const *const outfile)
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
	                  gitlab_get_apibase(),
	                  e_owner, e_repo, jid);

	gcli_curl(f, url, "application/zip");

	fclose(f);
	free(url);
	free(e_owner);
	free(e_repo);

	return rc;
}
