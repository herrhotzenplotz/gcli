/*
 * Copyright 2023 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#ifndef GCLI_GITLAB_MILESTONES_H
#define GCLI_GITLAB_MILESTONES_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gcli/milestones.h>

int gitlab_get_milestones(char const *owner,
                          char const *repo,
                          int max,
                          gcli_milestone_list *const out);

int gitlab_create_milestone(struct gcli_milestone_create_args const *args);

int gitlab_delete_milestone(char const *const owner,
                            char const *const repo,
                            int const milestone);

int gitlab_get_milestone(char const *owner,
                         char const *repo,
                         int const milestone,
                         gcli_milestone *const out);

int gitlab_milestone_get_issues(char const *const owner,
                                char const *const repo,
                                int const milestone,
                                gcli_issue_list *const out);

int gitlab_milestone_set_duedate(char const *const owner,
                                 char const *const repo,
                                 int const milestone,
                                 char const *const date);

#endif /* GCLI_GITLAB_MILESTONES_H */
