# Copyright 2021 Nico Sonack <nsonack@herrhotzenplotz.de>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

.SILENT:
.DEFAULT_TARGET: all

PROGS	?=	${PROG}
SRCS	?=	${${PROGS:=_SRCS}} ${${LIBS:=_SRCS}}
OBJS	=	${SRCS:.c=.o}
DEPS	=	${SRCS:.c=.d}

# The 'all' target needs to be put here such that is recognized as the
# default target on things like schily make
all: Makefile config.mk
	${MAKE} -f Makefile depend

# Unfortunately GNU make is a buggy piece of crap, so we have to do a
# dance around it and include stuff in this specific order (yes,
# otherwise GNU make is too fucking stupid to run the makefile at
# all). Even worse: it now tries to generate the dependency files
# first (which I didn't instruct it to do, but who cares right?) and
# then afterwards it runs the config.mk script which spits out the
# rules on how to properly generate the dependencies. At least it
# builds. This is the only way to do things without complete build
# failures and still maintain compatibility to OpenBSD make, as it
# does things seemingly correct. Mind you, it builds on all kinds of
# different make implementations. Only GNU make tries to be
# special. Thank you! </rant>
-include config.mk
-include ${DEPS}

config.mk: autodetect.sh
	rm -f config.mk
	@echo " ==> Performing autodetection of system variables"
	./autodetect.sh "${MAKE}"

.PHONY: build clean install depend ${MAN:=-install}
build: default.mk ${PROGS} ${LIBS}
	@echo " ==> Compilation finished"

depend: ${DEPS}
	@echo " ==> Starting build"
	${MAKE} -f Makefile build

.SUFFIXES: .c .d
.c.d:
	@echo " ==> Generating dependencies for $<"
	${CC} ${MKDEPS_FLAGS} $< > $@

.c.o:
	@echo " ==> Compiling $<"
	${CC} -c ${COMPILE_FLAGS} -o $@ $<

${PROGS}: ${OBJS} ${LIBS}
	@echo " ==> Linking $@"
	${LD} -o ${@} ${${@}_SRCS:.c=.o} ${LINK_FLAGS}

${LIBS}: ${OBJS}
	@echo " ==> Archiving $@"
	${AR} -rc ${@} ${${@}_SRCS:.c=.o}

clean:
	@echo " ==> Cleaning"
	@[ -d tests/ ] && ${MAKE} -C tests clean
	rm -f ${PROGS} ${LIBS} ${OBJS} ${DEPS} config.mk

PREFIX	?=	/usr/local
BINDIR	?=	${DESTDIR}${PREFIX}/bin
LIBDIR	?=	${DESTDIR}${PREFIX}/lib
MANDIR	?=	${DESTDIR}${PREFIX}/man

${PROGS:=-install}:
	@echo " ==> Installing program ${@:-install=}"
	install -d ${BINDIR}
	install ${@:-install=} ${BINDIR}

${MAN:=-install}:
	@echo " ==> Installing man page ${@:-install=}"
	install -d ${MANDIR}/man`echo "${@:-install=}" | sed 's/.*\.\([1-9]\)$$/\1/g'`
	gzip -c ${@:-install=} > ${MANDIR}/man`echo "${@:-install=}" | sed 's/.*\.\([1-9]\)$$/\1/g'`/`basename ${@:-install=}`.gz

install: all ${PROGS:=-install} ${MAN:=-install}
	@echo " ==> Installation finished"

snmk-libdeps:
	@echo ${LIBADD}

check: Makefile config.mk
	${MAKE} do-test

do-test: ${LIBS}
	${MAKE} -C tests
