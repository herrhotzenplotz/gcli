#include <atf-c.h>

#include <gcli/url.h>

#define URL_TC(name, url, _scheme, _user, _host, _port, _path)   \
ATF_TC_WITHOUT_HEAD(name);                                       \
ATF_TC_BODY(name, tc)                                            \
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
	ATF_REQUIRE(rc == 0);                                    \
                                                                 \
	if (expected.scheme)                                     \
		ATF_CHECK_STREQ(result.scheme, expected.scheme); \
	else                                                     \
	 	ATF_CHECK(result.scheme == NULL);                \
                                                                 \
	if (expected.user)                                       \
		ATF_CHECK_STREQ(result.user, expected.user);     \
	else                                                     \
	 	ATF_CHECK(result.user == NULL);                  \
                                                                 \
	if (expected.host)                                       \
		ATF_CHECK_STREQ(result.host, expected.host);     \
	else                                                     \
	 	ATF_CHECK(result.host == NULL);                  \
                                                                 \
	if (expected.port)                                       \
		ATF_CHECK_STREQ(result.port, expected.port);     \
	else                                                     \
	 	ATF_CHECK(result.port == NULL);                  \
                                                                 \
	if (expected.path)                                       \
		ATF_CHECK_STREQ(result.path, expected.path);     \
	else                                                     \
	 	ATF_CHECK(result.path == NULL);                  \
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

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, simple_http);
	ATF_TP_ADD_TC(tp, http_with_port);
	ATF_TP_ADD_TC(tp, simple_ssh);
	ATF_TP_ADD_TC(tp, simple_ssh_with_scheme);
	ATF_TP_ADD_TC(tp, ssh_with_port);

	return atf_no_error();
}
