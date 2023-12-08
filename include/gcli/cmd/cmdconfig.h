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

#ifndef GCLI_CMD_CMDCONFIG_H
#define GCLI_CMD_CMDCONFIG_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sn/sn.h>
#include <gcli/gcli.h>

#ifdef HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif /* HAVE_SYS_QUEUE_H */

struct gcli_config_entry {
	TAILQ_ENTRY(gcli_config_entry) next;
	sn_sv key;
	sn_sv value;
};

TAILQ_HEAD(gcli_config_entries, gcli_config_entry);

int gcli_config_parse_args(gcli_ctx *ctx, int *argc, char ***argv);
int gcli_config_init_ctx(gcli_ctx *ctx);
void gcli_config_get_upstream_parts(gcli_ctx *ctx, sn_sv *owner, sn_sv *repo);
char *gcli_config_get_apibase(gcli_ctx *);
sn_sv gcli_config_find_by_key(gcli_ctx *ctx, char const *section_name,
                              char const *key);

char *gcli_config_get_editor(gcli_ctx *ctx);
char *gcli_config_get_token(gcli_ctx *ctx);
char *gcli_config_get_account_name(gcli_ctx *ctx);
sn_sv gcli_config_get_upstream(gcli_ctx *ctx);
sn_sv gcli_config_get_base(gcli_ctx *ctx);
gcli_forge_type gcli_config_get_forge_type(gcli_ctx *ctx);
sn_sv gcli_config_get_override_default_account(gcli_ctx *ctx);
bool gcli_config_pr_inhibit_delete_source_branch(gcli_ctx *ctx);
void gcli_config_get_repo(gcli_ctx *ctx, char const **, char const **);
int gcli_config_have_colours(gcli_ctx *ctx);
struct gcli_config_entries const *gcli_config_get_section_entries(
	gcli_ctx *ctx, char const *section_name);

#endif /* GCLI_CMD_CMDCONFIG_H */
