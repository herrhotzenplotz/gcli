# Declare the list of programs
PROGS				=	ghcli
LIBS				=	libghcli.a

GHCLI_VERSION		=	0.9.0-beta
# These and LDFLAGS can be overwritten
CFLAGS				=	-std=iso9899:1999		\
						-Ithirdparty/pdjson/	\
						-Ithirdparty/			\
						-Iinclude/ -fPIC -fPIE
LDFLAGS				=	-L. -lghcli -rdynamic -fPIC \
						-fPIE
CFLAGS_amd64-freebsd-clang		=	-pedantic \
									-g -O0 -ggdb -Wall -Wextra
CFLAGS_sparc-sunos-sunstudio	=	-pedantic -I/opt/bw/include \
									-g -xO0
LDFLAGS_sparc-sunos-sunstudio	=	-L/opt/bw/lib -lcurl -R/opt/bw/lib
CPPFLAGS						=	-D_XOPEN_SOURCE=600 \
									-DGHCLI_VERSION_STRING=\"${GHCLI_VERSION}\"

# List the source files for each binary to be built
ghcli_SRCS			=	src/ghcli.c

libghcli.a_SRCS		=	src/comments.c				\
						src/config.c				\
						src/color.c					\
						src/curl.c					\
						src/editor.c				\
						src/forges.c				\
						src/forks.c					\
						src/gists.c					\
						src/labels.c				\
						src/gitconfig.c				\
						src/github/api.c			\
						src/github/checks.c			\
						src/github/comments.c		\
						src/github/config.c			\
						src/github/forks.c			\
						src/github/issues.c			\
						src/github/labels.c			\
						src/github/pulls.c			\
						src/github/releases.c		\
						src/github/repos.c			\
						src/github/review.c			\
						src/github/status.c			\
						src/gitlab/api.c			\
						src/gitlab/comments.c		\
						src/gitlab/config.c			\
						src/gitlab/forks.c			\
						src/gitlab/issues.c			\
						src/gitlab/labels.c			\
						src/gitlab/merge_requests.c	\
						src/gitlab/pipelines.c		\
						src/gitlab/releases.c		\
						src/gitlab/repos.c			\
						src/gitlab/review.c			\
						src/gitlab/status.c			\
						src/gitea/issues.c			\
						src/gitea/config.c			\
						src/gitea/comments.c		\
						src/gitea/labels.c			\
						src/gitea/pulls.c			\
						src/issues.c				\
						src/json_util.c				\
						src/pulls.c					\
						src/releases.c				\
						src/repos.c					\
						src/review.c				\
						src/snippets.c				\
						src/status.c				\
						thirdparty/pdjson/pdjson.c	\
						thirdparty/sn/sn.c

LIBADD				=	libcurl

# Leave this undefined if you don't have any manpages that need to be
# installed.
MAN				=	docs/ghcli.1			\
					docs/ghcli-repos.1		\
					docs/ghcli-forks.1		\
					docs/ghcli-releases.1	\
					docs/ghcli-issues.1		\
					docs/ghcli-pulls.1		\
					docs/ghcli-comment.1	\
					docs/ghcli-gists.1		\
					docs/ghcli-snippets.1	\
					docs/ghcli-status.1		\
					docs/ghcli-labels.1		\
					docs/ghcli-pipelines.1

# Include the rules to build your program
# Important: the autodetect.sh script needs to be in place
include default.mk

ghcli: libghcli.a

.PHONY: TAGS
TAGS:
	etags $$(find . -type f -name \*.[ch])
