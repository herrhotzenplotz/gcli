# Declare the list of programs
PROGS				=	ghcli

GHCLI_VERSION			=	0.2-alpha

# These and LDFLAGS can be overwritten
CFLAGS				=	-std=iso9899:1999 \
					-Ithirdparty/pdjson/ \
					-Ithirdparty/ \
					-Iinclude/
CFLAGS_amd64-freebsd-clang	=	-pedantic \
					-g -O0 -ggdb -Wall -Wextra
CFLAGS_sparc-sunos-sunstudio	=	-pedantic -I/opt/bw/include \
					-g -xO0
LDFLAGS_sparc-sunos-sunstudio	=	-L/opt/bw/lib -lcurl -R/opt/bw/lib
CPPFLAGS			=	-D_XOPEN_SOURCE=600 \
					-DGHCLI_VERSION_STRING=\"${GHCLI_VERSION}\"

# List the source files for each binary to be built
ghcli_SRCS			=	src/ghcli.c			\
					src/curl.c			\
					src/editor.c			\
					src/issues.c			\
					src/comments.c			\
					src/gitconfig.c			\
					src/pulls.c			\
					src/review.c			\
					src/json_util.c			\
					src/config.c			\
					thirdparty/sn/sn.c		\
					thirdparty/pdjson/pdjson.c

LIBADD				=	libcurl

# Leave this undefined if you don't have any manpages that need to be
# installed.
MAN				=	docs/ghcli.1

# Include the rules to build your program
# Important: the autodetect.sh script needs to be in place
include default.mk

.PHONY: TAGS
TAGS:
	etags $$(find . -type f -name \*.[ch])
