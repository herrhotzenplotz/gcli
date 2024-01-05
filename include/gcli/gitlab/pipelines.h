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

#ifndef GITLAB_PIPELINES_H
#define GITLAB_PIPELINES_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gcli/pulls.h>

typedef struct gitlab_pipeline      gitlab_pipeline;
typedef struct gitlab_pipeline_list gitlab_pipeline_list;
typedef struct gitlab_job           gitlab_job;
typedef struct gitlab_job_list      gitlab_job_list;

struct gitlab_pipeline {
	gcli_id id;
	char *status;
	char *created_at;
	char *updated_at;
	char *ref;
	char *sha;
	char *source;
};

struct gitlab_pipeline_list {
	gitlab_pipeline *pipelines;
	size_t pipelines_size;
};

struct gitlab_job {
	gcli_id id;
	char *status;
	char *stage;
	char *name;
	char *ref;
	char *created_at;
	char *started_at;
	char *finished_at;
	double duration;
	char *runner_name;
	char *runner_description;
	double coverage;
};

struct gitlab_job_list {
	gitlab_job *jobs;
	size_t jobs_size;
};

int gitlab_get_pipelines(struct gcli_ctx *ctx, char const *owner,
                         char const *repo, int max, gitlab_pipeline_list *out);

void gitlab_pipeline_free(gitlab_pipeline *pipeline);
void gitlab_pipelines_free(gitlab_pipeline_list *list);

int gitlab_get_pipeline_jobs(struct gcli_ctx *ctx, char const *owner,
                             char const *repo, gcli_id pipeline, int count,
                             gitlab_job_list *out);

void gitlab_free_jobs(gitlab_job_list *jobs);
void gitlab_free_job(gitlab_job *job);

int gitlab_job_get_log(struct  gcli_ctx *ctx, char const *owner,
                       char const *repo, gcli_id job_id, FILE *stream);

int gitlab_job_cancel(struct gcli_ctx *ctx, char const *owner, char const *repo,
                      gcli_id job_id);

int gitlab_job_retry(struct gcli_ctx *ctx, char const *owner, char const *repo,
                     gcli_id job_id);

int gitlab_job_download_artifacts(struct gcli_ctx *ctx, char const *owner,
                                  char const *repo, gcli_id jid,
                                  char const *outfile);

int gitlab_get_mr_pipelines(struct gcli_ctx *ctx, char const *owner,
                            char const *repo, gcli_id mr_id,
                            gitlab_pipeline_list *list);

int gitlab_get_job(struct gcli_ctx *ctx, char const *owner, char const *repo,
                   gcli_id const jid, gitlab_job *const out);

#endif /* GITLAB_PIPELINES_H */
