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

#include <gcli/url.h>

#include "unit.h"

#define URL_TC(name, url, _scheme, _user, _host, _port, _path)   \
DEFINE_TESTCASE(name)                                            \
{                                                                \
	char in[] = url;                                         \
	int rc = 0;                                              \
	struct gcli_url result = {0};                            \
	struct gcli_url const expected = {                       \
		.scheme = _scheme,                               \
		.user = _user,                                   \
		.host = _host,                                   \
		.port = _port,                                   \
		.path = _path,                                   \
	};                                                       \
                                                                 \
	rc = gcli_parse_url(in, &result);                        \
	REQUIRE(rc == 0);                                        \
                                                                 \
	if (expected.scheme)                                     \
		CHECK_STREQ(result.scheme, expected.scheme);     \
	else                                                     \
	 	CHECK(result.scheme == NULL);                    \
                                                                 \
	if (expected.user)                                       \
		CHECK_STREQ(result.user, expected.user);         \
	else                                                     \
	 	CHECK(result.user == NULL);                      \
                                                                 \
	if (expected.host)                                       \
		CHECK_STREQ(result.host, expected.host);         \
	else                                                     \
	 	CHECK(result.host == NULL);                      \
                                                                 \
	if (expected.port)                                       \
		CHECK_STREQ(result.port, expected.port);         \
	else                                                     \
	 	CHECK(result.port == NULL);                      \
                                                                 \
	if (expected.path)                                       \
		CHECK_STREQ(result.path, expected.path);         \
	else                                                     \
	 	CHECK(result.path == NULL);                      \
                                                                 \
	gcli_url_free(&result);                                  \
}

URL_TC(simple_http, "https://git.sr.ht/~herrhotzenplotz/gcli",
       "https", NULL, "git.sr.ht", NULL, "~herrhotzenplotz/gcli")

URL_TC(simple_ssh, "git@github.com:herrhotzenplotz/gcli",
       NULL, "git", "github.com", NULL, "herrhotzenplotz/gcli")

URL_TC(http_with_port, "https://git.foo.bar:4242/barf/bork/blerch",
       "https", NULL, "git.foo.bar", "4242", "barf/bork/blerch")

URL_TC(simple_ssh_with_scheme, "ssh://git@github.com/herrhotzenplotz/gcli",
       "ssh", "git", "github.com", NULL, "herrhotzenplotz/gcli")

URL_TC(ssh_with_port, "ssh://git@git.example.com:4242/herrhotzenplotz/gcli",
       "ssh", "git", "git.example.com", "4242", "herrhotzenplotz/gcli")

URL_TC(user_and_host, "git@git.example.com",
       NULL, "git", "git.example.com", NULL, NULL)

URL_TC(fs_path, "/some/path/in/here",
       NULL, NULL, NULL, NULL, "/some/path/in/here")

TESTSUITE
{
	TESTCASE(simple_http);
	TESTCASE(http_with_port);
	TESTCASE(simple_ssh);
	TESTCASE(simple_ssh_with_scheme);
	TESTCASE(ssh_with_port);
	TESTCASE(user_and_host);
	TESTCASE(fs_path);
}
