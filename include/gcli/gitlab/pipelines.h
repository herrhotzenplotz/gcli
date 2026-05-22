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

#ifndef GITLAB_PIPELINES_H
#define GITLAB_PIPELINES_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gcli/pipelines.h>

int gitlab_get_pipelines(struct gcli_ctx *ctx,
                         struct gcli_path const *repo_path,
                         struct gcli_pipelines_fetch_details const *details,
                         struct gcli_pipeline_list *out);

int gitlab_get_pipeline(struct gcli_ctx *ctx,
                        struct gcli_path const *pipeline_path,
                        struct gcli_pipeline *out);

int gitlab_get_pipeline_jobs(struct gcli_ctx *ctx,
                             struct gcli_path const *pipeline_path,
                             int count, struct gcli_job_list *out);

int gitlab_get_pipeline_children(struct gcli_ctx *ctx,
                                 struct gcli_path const *pipeline_path,
                                 int count, struct gcli_pipeline_list *out);

int gitlab_job_get_log(struct  gcli_ctx *ctx, struct gcli_path const *job_path,
                       FILE *stream);

int gitlab_job_cancel(struct gcli_ctx *ctx, struct gcli_path const *job_path);

int gitlab_job_retry(struct gcli_ctx *ctx, struct gcli_path const *job_path);

int gitlab_job_download_artifacts(struct gcli_ctx *ctx,
                                  struct gcli_path const *job_path,
                                  char const *outfile);

int gitlab_get_mr_pipelines(struct gcli_ctx *ctx, struct gcli_path const *path,
                            struct gcli_pipeline_list *list);

int gitlab_get_job(struct gcli_ctx *ctx, struct gcli_path const *job_path,
                   struct gcli_job *const out);

#endif /* GITLAB_PIPELINES_H */
