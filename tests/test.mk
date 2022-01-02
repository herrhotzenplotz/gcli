CFLAGS				=	-I../../include	\
					-I../../thirdparty
LDFLAGS_sparc-sunos-sunstudio	=	-L/opt/bw/lib -lcurl -R/opt/bw/lib

-include ../../config.mk

.SILENT:

# Force testing again if we run the test suite
.PHONY: test ${TESTNAME}

test: ${TESTNAME}
	./${TESTNAME} > /tmp/ghcli-test-stdout.${TESTNAME} 2>/tmp/ghcli-test-stderr.${TESTNAME}
	@diff -u expected.stdout /tmp/ghcli-test-stdout.${TESTNAME} || (echo "[FAIL] Unexpected stdout of ${TESTNAME}" && exit 1)
	@diff -u expected.stderr /tmp/ghcli-test-stderr.${TESTNAME} || (echo "[FAIL] Unexpected stderr of ${TESTNAME}" && exit 1)
	@echo "[SUCCESS] ${TESTNAME}"

${TESTNAME}: ${TESTNAME}.o
	${CC} -o ${TESTNAME} ${LINK_FLAGS} ${TESTNAME}.o ../../libghcli.a

${TESTNAME}.o: ${TESTNAME}.c
	${CC} -c -o ${TESTNAME}.o ${COMPILE_FLAGS} ${TESTNAME}.c

clean:
	rm -f /tmp/ghcli-test-stdout.${TESTNAME}
	rm -f /tmp/ghcli-test-stderr.${TESTNAME}
	rm -f ${TESTNAME}.o
	rm -f ${TESTNAME}
