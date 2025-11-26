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
