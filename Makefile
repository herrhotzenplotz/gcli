# Declare the list of programs
PROGS				=	ghcli
LIBS				=	libghcli.a

GHCLI_VERSION			=	0.6.0-alpha
# These and LDFLAGS can be overwritten
CFLAGS				=	-std=iso9899:1999 \
					-Ithirdparty/pdjson/ \
					-Ithirdparty/ \
					-Iinclude/ -fPIC -fPIE
LDFLAGS				=	-L. -lghcli -rdynamic -fPIC \
					-fPIE
CFLAGS_amd64-freebsd-clang	=	-pedantic \
					-g -O0 -ggdb -Wall -Wextra
CFLAGS_sparc-sunos-sunstudio	=	-pedantic -I/opt/bw/include \
					-g -xO0
LDFLAGS_sparc-sunos-sunstudio	=	-L/opt/bw/lib -lcurl -R/opt/bw/lib
CPPFLAGS			=	-D_XOPEN_SOURCE=600 \
					-DGHCLI_VERSION_STRING=\"${GHCLI_VERSION}\"

# List the source files for each binary to be built
ghcli_SRCS			=	src/ghcli.c			\
					src/editor.c

libghcli.a_SRCS			=	src/comments.c			\
					src/config.c			\
					src/curl.c			\
					src/forges.c			\
					src/forks.c			\
					src/gists.c			\
					src/gitconfig.c			\
					src/github/api.c		\
					src/github/comments.c		\
					src/github/config.c		\
					src/github/forks.c		\
					src/github/issues.c		\
					src/github/pulls.c		\
					src/github/releases.c		\
					src/github/repos.c		\
					src/github/review.c		\
					src/gitlab/api.c		\
					src/gitlab/comments.c		\
					src/gitlab/config.c		\
					src/gitlab/forks.c		\
					src/gitlab/issues.c		\
					src/gitlab/merge_requests.c	\
					src/gitlab/releases.c		\
					src/gitlab/repos.c		\
					src/gitlab/review.c		\
					src/issues.c			\
					src/issues.c			\
					src/json_util.c			\
					src/pulls.c			\
					src/releases.c			\
					src/repos.c			\
					src/review.c			\
					src/snippets.c			\
					thirdparty/pdjson/pdjson.c	\
					thirdparty/sn/sn.c

LIBADD				=	libcurl

# Leave this undefined if you don't have any manpages that need to be
# installed.
MAN				=	docs/ghcli.1 \
					docs/ghcli-repos.1 \
					docs/ghcli-forks.1 \
					docs/ghcli-releases.1 \
					docs/ghcli-issues.1 \
					docs/ghcli-pulls.1 \
					docs/ghcli-comment.1 \
					docs/ghcli-gists.1 \
					docs/ghcli-snippets.1

# Include the rules to build your program
# Important: the autodetect.sh script needs to be in place
include default.mk

ghcli: libghcli.a

.PHONY: TAGS
TAGS:
	etags $$(find . -type f -name \*.[ch])
