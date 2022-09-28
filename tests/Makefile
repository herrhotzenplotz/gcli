TESTS	=	json-escape \
			url-encode \
			github-tests

LDFLAGS_sparc-sunos-sunstudio	+=	-L/opt/bw/lib -lcurl -R/opt/bw/lib
CFLAGS							+=	-I../include -I../thirdparty
LDFLAGS							+=	-latf-c

-include ../config.mk

.PHONY: all clean ${TESTS:=-clean}

all: build-tests run-tests

clean:
	rm -f ${TESTS}

build-tests: ${TESTS}

.c:
	${CC} -o $@ ${COMPILE_FLAGS} $< ../libgcli.a ${LINK_FLAGS}

run-tests: Kyuafile ${TESTS}
	kyua test
