# Copyright 2021 Nico Sonack <nsonack@outlook.com>
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

-include config.mk

PROGS	?=	${PROG}
SRCS	?=	${${PROGS:=_SRCS}}
OBJS	=	${SRCS:.c=.o}

all: config.mk
	${MAKE} -f Makefile build

config.mk: autodetect.sh
	rm -f config.mk
	./autodetect.sh

.PHONY: build clean install ${MAN:=-install}
build: default.mk Makefile ${PROGS}

.c.o:
	${CC} -c ${COMPILE_FLAGS} -o $@ $<

${PROGS}: ${OBJS}
	${LD} ${LINK_FLAGS} -o ${@} ${${@}_SRCS:.c=.o}

clean:
	rm -f ${PROGS} ${OBJS} config.mk

PREFIX	?=	/usr/local
DESTDIR	?=	/
BINDIR	?=	${DESTDIR}/${PREFIX}/bin
MANDIR	?=	${DESTDIR}/${PREFIX}/man

${PROGS:=-install}:
	install -d ${BINDIR}
	install ${@:-install=} ${BINDIR}

${MAN:=-install}:
	install -d ${MANDIR}/man`echo "${@:-install=}" | sed 's/.*\.\([1-9]\)$$/\1/g'`
	gzip -c ${@:-install=} > ${MANDIR}/man`echo "${@:-install=}" | sed 's/.*\.\([1-9]\)$$/\1/g'`/`basename ${@:-install=}`.gz

install: all ${PROGS:=-install} ${MAN:=-install}
