TESTS	=	json_escape \
		url-encode

.PHONY: all clean ${TESTS:=-clean} ${TESTS}

all: ${TESTS}

clean: ${TESTS:=-clean}

${TESTS}:
	${MAKE} -C ${@} test

${TESTS:=-clean}:
	${MAKE} -C ${@:-clean=} clean
