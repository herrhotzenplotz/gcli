/*
 * Copyright 2021 Nico Sonack <nsonack@outlook.com>
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

#include <ghcli/config.h>
#include <ghcli/gitlab/config.h>

#include <sn/sn.h>

static sn_sv
gitlab_default_account_name(void)
{
    sn_sv section_name = ghcli_config_find_by_key(
        SV("defaults"),
        "gitlab-default-account");

    if (sn_sv_null(section_name))
        errx(1, "Config file does not name a default GitLab account name.");

    return section_name;
}

char *
gitlab_get_apibase(void)
{
    sn_sv account  = gitlab_default_account_name();
    sn_sv api_base = ghcli_config_find_by_key(account, "apibase");

    if (sn_sv_null(api_base))
        return "https://gitlab.com/api/v4";

    return sn_sv_to_cstr(api_base);
}

char *
gitlab_get_authheader(void)
{
    sn_sv account = gitlab_default_account_name();
    sn_sv token = ghcli_config_find_by_key(account, "token");
    if (sn_sv_null(token))
        errx(1, "Missing GitLab token");
    return sn_asprintf("PRIVATE-TOKEN: "SV_FMT, SV_ARGS(token));
}

sn_sv
gitlab_get_account(void)
{
    sn_sv section = gitlab_default_account_name();
    sn_sv account = ghcli_config_find_by_key(section, "account");;
    if (sn_sv_null(account))
        errx(1, "Missing GitLab account name");
    return account;
}
