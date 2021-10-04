# Declare the list of programs
PROGS				=	ghcli

# These and LDFLAGS can be overwritten
CFLAGS				=	-std=iso9899:1999
CFLAGS_amd64-freebsd-clang	=	-pedantic
CPPFLAGS			=	-D_XOPEN_SOURCE=600

# List the source files for each binary to be built
ghcli_SRCS			=	src/ghcli.c

# Leave this undefined if you don't have any manpages that need to be
# installed.
# MAN				=	docs/foo.8

# Include the rules to build your program
# Important: the autodetect.sh script needs to be in place
include default.mk
