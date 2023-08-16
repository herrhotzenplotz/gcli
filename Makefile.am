ACLOCAL_AMFLAGS = -I m4

AM_YFLAGS = -d

AM_CPPFLAGS = -I$(srcdir)/include
AM_CPPFLAGS += -I$(srcdir)/thirdparty
AM_CPPFLAGS += -I$(srcdir)
AM_CPPFLAGS += -DYY_NO_INPUT
AM_CPPFLAGS += -DYY_NO_UNPUT
AM_CPPFLAGS += -D_XOPEN_SOURCE=600

noinst_PROGRAMS = pgen$(EXEEXT)
pgen_SOURCES = \
	include/gcli/pgen.h \
	src/pgen/dump_c.c \
	src/pgen/dump_h.c \
	src/pgen/dump_plain.c \
	src/pgen/parser.y \
	src/pgen/lexer.l

lib_LTLIBRARIES = libgcli.la
noinst_LTLIBRARIES = libsn.la libpdjson.la

.NOTPARALLEL: pgen$(EXEEXT) $(builddir)/src/pgen/parser.c $(builddir)/src/pgen/parser.h $(builddir)/src/pgen/lexer.c

libgcli_la_DEPENDENCIES = pgen$(EXEEXT) libpdjson.la libsn.la

$(BUILT_SOURCES): pgen$(EXEEXT)

SUFFIXES = .t
.t.h:
	$(AM_V_GEN)./pgen -th -o $@ $<

.t.c:
	$(AM_V_GEN)./pgen -tc -o $@ $<

bin_PROGRAMS = gcli$(EXEEXT)
gcli_LDADD = libgcli.la

libgcli_la_LIBADD = libpdjson.la libsn.la

dist_man_MANS = \
	docs/gcli-api.1 \
	docs/gcli-comment.1 \
	docs/gcli-config.1 \
	docs/gcli-forks.1 \
	docs/gcli-gists.1 \
	docs/gcli-issues.1 \
	docs/gcli-labels.1 \
	docs/gcli-milestones.1 \
	docs/gcli-pipelines.1 \
	docs/gcli-pulls.1 \
	docs/gcli-releases.1 \
	docs/gcli-repos.1 \
	docs/gcli-snippets.1 \
	docs/gcli-status.1 \
	docs/gcli.1

gcli_SOURCES = \
	include/gcli/cmd/cmd.h src/cmd/cmd.c \
	include/gcli/cmd/table.h src/cmd/table.c \
	src/cmd/api.c \
	src/cmd/ci.c \
	src/cmd/comment.c \
	src/cmd/config.c \
	src/cmd/forks.c \
	src/cmd/gcli.c \
	src/cmd/gists.c \
	src/cmd/issues.c \
	src/cmd/labels.c \
	src/cmd/milestones.c \
	src/cmd/pipelines.c \
	src/cmd/pulls.c \
	src/cmd/releases.c \
	src/cmd/repos.c \
	src/cmd/snippets.c \
	src/cmd/status.c

libsn_la_SOURCES = \
	thirdparty/sn/sn.c \
	thirdparty/sn/sn.h

libpdjson_la_SOURCES = \
	thirdparty/pdjson/pdjson.c \
	thirdparty/pdjson/pdjson.h

TEMPLATES = \
	templates/github/checks.t \
	templates/github/comments.t \
	templates/github/forks.t \
	templates/github/gists.t \
	templates/github/issues.t \
	templates/github/labels.t \
	templates/github/pulls.t \
	templates/github/releases.t \
	templates/github/repos.t \
	templates/github/status.t \
	templates/github/milestones.t \
	templates/gitlab/comments.t \
	templates/gitlab/forks.t \
	templates/gitlab/issues.t \
	templates/gitlab/labels.t \
	templates/gitlab/merge_requests.t \
	templates/gitlab/milestones.t \
	templates/gitlab/pipelines.t \
	templates/gitlab/releases.t \
	templates/gitlab/repos.t \
	templates/gitlab/review.t \
	templates/gitlab/sshkeys.t \
	templates/gitlab/status.t \
	templates/gitlab/snippets.t \
	templates/gitea/milestones.t

include_HEADERS = \
	include/gcli/gcli.h include/gcli/comments.h \
	include/gcli/config.h include/gcli/curl.h \
	include/gcli/editor.h include/gcli/forks.h \
	include/gcli/gitconfig.h include/gcli/issues.h \
	include/gcli/labels.h include/gcli/milestones.h \
	include/gcli/pulls.h include/gcli/releases.h \
	include/gcli/repos.h include/gcli/review.h \
	include/gcli/gitlab/snippets.h include/gcli/status.h \
	include/gcli/sshkeys.h

libgcli_la_SOURCES = \
	include/gcli/ctx.h src/ctx.c \
	include/gcli/gcli.h src/gcli.c \
	src/colour.c include/gcli/colour.h \
	src/comments.c include/gcli/comments.h \
	src/config.c include/gcli/config.h \
	src/curl.c include/gcli/curl.h \
	src/editor.c include/gcli/editor.h \
	src/forges.c include/gcli/forges.h \
	src/forks.c include/gcli/forks.h \
	src/gitconfig.c include/gcli/gitconfig.h \
	src/issues.c include/gcli/issues.h \
	src/json_util.c include/gcli/json_util.h \
	src/labels.c include/gcli/labels.h \
	src/milestones.c include/gcli/milestones.h \
	src/pulls.c include/gcli/pulls.h \
	src/releases.c include/gcli/releases.h \
	src/repos.c include/gcli/repos.h \
	src/review.c include/gcli/review.h \
	src/gitlab/snippets.c include/gcli/gitlab/snippets.h \
	src/status.c include/gcli/status.h \
	src/sshkeys.c include/gcli/sshkeys.h \
	src/gitlab/api.c include/gcli/gitlab/api.h \
	src/gitlab/comments.c include/gcli/gitlab/comments.h \
	src/gitlab/config.c include/gcli/gitlab/config.h \
	src/gitlab/forks.c include/gcli/gitlab/forks.h \
	src/gitlab/issues.c include/gcli/gitlab/issues.h \
	src/gitlab/labels.c include/gcli/gitlab/labels.h \
	src/gitlab/merge_requests.c include/gcli/gitlab/merge_requests.h \
	src/gitlab/milestones.c include/gcli/gitlab/milestones.h \
	src/gitlab/pipelines.c include/gcli/gitlab/pipelines.h \
	src/gitlab/releases.c include/gcli/gitlab/releases.h \
	src/gitlab/repos.c include/gcli/gitlab/repos.h \
	src/gitlab/review.c include/gcli/gitlab/review.h \
	src/gitlab/status.c include/gcli/gitlab/status.h \
	src/gitlab/sshkeys.c include/gcli/gitlab/sshkeys.h \
	src/github/releases.c include/gcli/github/releases.h \
	src/github/config.c include/gcli/github/config.h \
	src/github/api.c include/gcli/github/api.h \
	src/github/repos.c include/gcli/github/repos.h \
	src/github/forks.c include/gcli/github/forks.h \
	src/github/pulls.c include/gcli/github/pulls.h \
	src/github/comments.c include/gcli/github/comments.h \
	src/github/status.c include/gcli/github/status.h \
	src/github/labels.c include/gcli/github/labels.h \
	src/github/milestones.c include/gcli/github/milestones.h \
	src/github/issues.c include/gcli/github/issues.h \
	src/github/review.c include/gcli/github/review.h \
	src/github/checks.c include/gcli/github/checks.h \
	src/github/gists.c include/gcli/github/gists.h \
	src/github/sshkeys.c include/gcli/github/sshkeys.h \
	src/gitea/issues.c include/gcli/gitea/issues.h \
	src/gitea/labels.c include/gcli/gitea/labels.h \
	src/gitea/forks.c include/gcli/gitea/forks.h \
	src/gitea/comments.c include/gcli/gitea/comments.h \
	src/gitea/config.c include/gcli/gitea/config.h \
	src/gitea/pulls.c include/gcli/gitea/pulls.h \
	src/gitea/releases.c include/gcli/gitea/releases.h \
	src/gitea/repos.c include/gcli/gitea/repos.h \
	src/gitea/sshkeys.c include/gcli/gitea/sshkeys.h \
	src/gitea/milestones.c include/gcli/gitea/milestones.h \
	$(TEMPLATES)

libgcli_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-DIN_LIBGCLI=1

BUILT_SOURCES = \
	$(TEMPLATES:.t=.c) \
	$(TEMPLATES:.t=.h)

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
	LICENSE \
	README.md \
	HACKING.md \
	autogen.sh \
	m4/.gitkeep

###########################################
# Tests
check_PROGRAMS = \
	tests/json-escape$(EXEEXT) \
	tests/pgen-tests$(EXEEXT) \
	tests/url-encode$(EXEEXT) \
	tests/pretty_print_test$(EXEEXT)

SHELL_TESTS = \
	tests/01_pgen_simple_object.sh \
	tests/02_pgen_error_no_entries.sh \
	tests/03_pgen_error_trailing_comma.sh \
	tests/04_pgen_simple_issue.sh \
	tests/05_pgen_simple_pull.sh \
	tests/06_pgen_simple_label.sh \
	tests/07_pretty_print.sh

EXTRA_DIST += \
	${SHELL_TESTS} \
	tests/setup.sh \
	tests/samples/pgen_simple_object.dump \
	tests/samples/github_simple_issue.json tests/samples/parse_simple_issue.dump \
	tests/samples/github_simple_pull.json tests/samples/parse_simple_pull.dump \
	tests/samples/github_simple_label.json tests/samples/parse_simple_label.dump \
	tests/samples/pretty.dump

TESTS = \
	tests/json-escape \
	tests/url-encode \
	${SHELL_TESTS}

tests_json_escape_LDADD = \
	libgcli.la libpdjson.la libsn.la
tests_pgen_tests_LDADD = \
	libgcli.la libpdjson.la libsn.la
tests_url_encode_LDADD = \
	libgcli.la libpdjson.la libsn.la
tests_pretty_print_test_LDADD = \
	libsn.la
