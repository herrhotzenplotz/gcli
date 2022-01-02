TESTS	=	json_escape

.PHONY: all clean ${TESTS:=-clean} ${TESTS}

all: ${TESTS}

clean: ${TESTS:=-clean}

${TESTS}:
	${MAKE} -C ${@} test

${TESTS:=-clean}:
	${MAKE} -C ${@:-clean=} clean
