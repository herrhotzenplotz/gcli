# If you are having problems compiling gcli, try to build an SCU
# version:
#
#    cc -O3 -o gcli -Iinclude -Ithirdparty \
#    -DGCLI_VERSION_STRING="\"Manual SCU build\"" \
#    -I/usr/local/include `find . -type f -name \*.c | grep -v test` \
#    -L/usr/local/lib -lcurl
#
#    Adapt the linker- and include paths to your system.
#
#  Please report such cases to me. If the problem arises from GNU
#  make, please see my rant in default.mk. Likely you want to fix the
#  problem by using something other than GNU make.

# Declare the list of programs
PROGS							=	gcli
LIBS							=	libgcli.a

GCLI_VERSION					=	0.9.7-beta

#########################################################################
# CPPFLAGS
CPPFLAGS						=	-D_XOPEN_SOURCE=600 \
									-DGCLI_VERSION_STRING=\"${GCLI_VERSION}\"

#########################################################################
# CFLAGS
CFLAGS							=	-std=iso9899:1999		\
									-Ithirdparty/pdjson/	\
									-Ithirdparty/			\
									-Iinclude/
CFLAGS_gcc						=	-fPIC -fPIE -pedantic -g -O0 -ggdb \
									-Wall -Wextra
CFLAGS_clang					=	${CFLAGS_gcc}

CFLAGS_sparc-sunos-sunstudio	=	-pedantic -I/opt/bw/include \
									-g -xO0

#########################################################################
# LDFLAGS
LDFLAGS							=	-L. -lgcli
LDFLAGS_gcc						=	-rdynamic -fPIC -fPIE
LDFLAGS_clang					=	${LDFLAGS_gcc}
LDFLAGS_sparc-sunos-sunstudio	=	-L/opt/bw/lib -lcurl -R/opt/bw/lib

#########################################################################
# List the source files for each binary to be built
gcli_SRCS			=	src/gcli.c

libgcli.a_SRCS		=	src/comments.c				\
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
						src/gitea/repos.c			\
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
MAN				=	docs/gcli.1				\
					docs/gcli-repos.1		\
					docs/gcli-forks.1		\
					docs/gcli-releases.1	\
					docs/gcli-issues.1		\
					docs/gcli-pulls.1		\
					docs/gcli-comment.1		\
					docs/gcli-gists.1		\
					docs/gcli-snippets.1	\
					docs/gcli-status.1		\
					docs/gcli-labels.1		\
					docs/gcli-pipelines.1

# Include the rules to build your program
# Important: the autodetect.sh script needs to be in place
include default.mk

gcli: libgcli.a

.PHONY: TAGS
TAGS:
	etags $$(find . -type f -name \*.[ch])
