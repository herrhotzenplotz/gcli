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

#include <ghcli/gitea/config.h>
#include <ghcli/config.h>

#include <sn/sn.h>

static sn_sv
gitea_default_account_name(void)
{
	sn_sv section_name;

	/* Use default override account */
	section_name = ghcli_config_get_override_default_account();

	/* If not manually overridden */
	if (sn_sv_null(section_name)) {
		section_name = ghcli_config_find_by_key(
			SV("defaults"),
			"gitea-default-account");

		/* Welp, no luck here */
		if (sn_sv_null(section_name))
			warnx("Config file does not name a default Gitea account name.");
	}

	return section_name;
}

const char *
gitea_get_apibase(void)
{
	sn_sv account = gitea_default_account_name();
	if (sn_sv_null(account))
		goto default_val;

	sn_sv api_base = ghcli_config_find_by_key(account, "apibase");
	if (sn_sv_null(api_base))
		goto default_val;

	return sn_sv_to_cstr(api_base);

default_val:
	return "https://codeberg.org/api/v1";
}

char *
gitea_get_authheader(void)
{
	sn_sv account = gitea_default_account_name();
	if (sn_sv_null(account))
		return NULL;

	sn_sv token = ghcli_config_find_by_key(account, "token");;
	if (sn_sv_null(token)) {
		warnx("Missing Gitea token");
		return NULL;
	}

	return sn_asprintf("Authorization: token "SV_FMT, SV_ARGS(token));
}

sn_sv
gitea_get_account(void)
{
	sn_sv section = gitea_default_account_name();
	if (sn_sv_null(section))
		return SV_NULL;

	sn_sv account = ghcli_config_find_by_key(section, "account");;
	if (!account.length)
		errx(1, "Missing Gitea account name");
	return account;
}

