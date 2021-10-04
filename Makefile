# Declare the list of programs
PROGS				=	ghcli

# These and LDFLAGS can be overwritten
CFLAGS				=	-std=iso9899:1999 \
					-Ithirdparty/pdjson/ \
					-Ithirdparty/ \
					-Iinclude/
CFLAGS_amd64-freebsd-clang	=	-pedantic -I/usr/local/include \
					-g -O0 -ggdb
LDFLAGS_amd64-freebsd-clang	=	-L/usr/local/lib -lcurl
CPPFLAGS			=	-D_XOPEN_SOURCE=600

# List the source files for each binary to be built
ghcli_SRCS			=	src/ghcli.c \
					thirdparty/sn/sn.c \
					thirdparty/pdjson/pdjson.c

# Leave this undefined if you don't have any manpages that need to be
# installed.
# MAN				=	docs/foo.8

# Include the rules to build your program
# Important: the autodetect.sh script needs to be in place
include default.mk
