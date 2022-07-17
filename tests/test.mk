CFLAGS				=	-I../../include	\
					-I../../thirdparty
LDFLAGS_sparc-sunos-sunstudio	=	-L/opt/bw/lib -lcurl -R/opt/bw/lib

-include ../../config.mk

.SILENT:

# Force testing again if we run the test suite
.PHONY: test ${TESTNAME}

test: ${TESTNAME}
	./${TESTNAME} > /tmp/gcli-test-stdout.${TESTNAME} 2>/tmp/gcli-test-stderr.${TESTNAME}
	@diff -u expected.stdout /tmp/gcli-test-stdout.${TESTNAME} || (echo "[FAIL] Unexpected stdout of ${TESTNAME}" && exit 1)
	@diff -u expected.stderr /tmp/gcli-test-stderr.${TESTNAME} || (echo "[FAIL] Unexpected stderr of ${TESTNAME}" && exit 1)
	@echo "[SUCCESS] ${TESTNAME}"

${TESTNAME}: ${TESTNAME}.o
	${CC} -o ${TESTNAME} ${TESTNAME}.o ../../libgcli.a ${LINK_FLAGS}

${TESTNAME}.o: ${TESTNAME}.c
	${CC} -c -o ${TESTNAME}.o ${COMPILE_FLAGS} ${TESTNAME}.c

clean:
	rm -f /tmp/gcli-test-stdout.${TESTNAME}
	rm -f /tmp/gcli-test-stderr.${TESTNAME}
	rm -f ${TESTNAME}.o
	rm -f ${TESTNAME}
