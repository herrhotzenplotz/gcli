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
#include "config.h"
#endif

typedef struct gitlab_pipeline gitlab_pipeline;
typedef struct gitlab_job      gitlab_job;

struct gitlab_pipeline {
    long  id;
    char *status;
    char *created_at;
    char *updated_at;
    char *ref;
    char *sha;
    char *source;
};

struct gitlab_job {
    long    id;
    char   *status;
    char   *stage;
    char   *name;
    char   *ref;
    char   *created_at;
    char   *started_at;
    char   *finished_at;
    double  duration;
    char   *runner_name;
    char   *runner_description;
};

int gitlab_get_pipelines(char const *owner,
                         char const *repo,
                         int const max,
                         gitlab_pipeline **const out);

void gitlab_print_pipelines(gitlab_pipeline const *const pipelines,
                            int const pipelines_size);

void gitlab_free_pipelines(gitlab_pipeline *pipelines,
                           int const pipelines_size);

void gitlab_pipelines(char const *owner,
                      char const *repo,
                      int const count);

void gitlab_pipeline_jobs(char const *owner,
                          char const *repo,
                          long const pipeline,
                          int const count);

int gitlab_get_pipeline_jobs(char const *owner,
                             char const *repo,
                             long const pipeline,
                             int const count,
                             gitlab_job **const jobs);

void gitlab_print_jobs(gitlab_job const *const jobs,
                       int const jobs_size);

void gitlab_free_jobs(gitlab_job *jobs,
                      int const jobs_size);

void gitlab_job_get_log(char const *owner,
                        char const *repo,
                        long const job_id);

void gitlab_job_status(char const *owner,
                       char const *repo,
                       long const job_id);

void gitlab_job_cancel(char const *owner,
                       char const *repo,
                       long const job_id);

void gitlab_job_retry(char const *owner,
                      char const *repo,
                      long const job_id);

void gitlab_mr_pipelines(char const *owner,
                         char const *repo,
                         long const mr_id);

#endif /* GITLAB_PIPELINES_H */
