/*
 * Copyright 2021, 2022 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#ifndef ISSUES_H
#define ISSUES_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sn/sn.h>
#include <gcli/gcli.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct gcli_issue gcli_issue;
typedef struct gcli_submit_issue_options gcli_submit_issue_options;

struct gcli_issue {
    int     number;
    sn_sv   title;
    sn_sv   created_at;
    sn_sv   author;
    sn_sv   state;
    int     comments;
    bool    locked;
    sn_sv   body;
    sn_sv  *labels;
    size_t  labels_size;
    sn_sv  *assignees;
    size_t  assignees_size;
};

struct gcli_submit_issue_options {
    sn_sv owner;
    sn_sv repo;
    sn_sv title;
    sn_sv body;
    bool  always_yes;
};

int  gcli_get_issues(
    const char  *owner,
    const char  *reponame,
    bool         all,
    int          max,
    gcli_issue **out);
void gcli_issues_free(
    gcli_issue *it,
    int         size);
void gcli_print_issues_table(
    enum gcli_output_order  order,
    gcli_issue             *issues,
    int                     issues_size);
void gcli_issue_summary(
    const char *owner,
    const char *reponame,
    int         issue_number);
void gcli_issue_close(
    const char *owner,
    const char *repo,
    int         issue_number);
void gcli_issue_reopen(
    const char *owner,
    const char *repo,
    int         issue_number);
void gcli_issue_submit(gcli_submit_issue_options);
void gcli_issue_assign(
    const char *owner,
    const char *repo,
    int         issue_number,
    const char *assignee);
void gcli_issue_add_labels(
    const char *owner,
    const char *repo,
    int         issue_number,
    const char *labels[],
    size_t      labels_size);
void gcli_issue_remove_labels(
    const char *owner,
    const char *repo,
    int         issue_number,
    const char *labels[],
    size_t      labels_size);

#endif /* ISSUES_H */
