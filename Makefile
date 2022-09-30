.PHONY: all clean check
all:
	./build.sh

clean:
	@echo "Use git clean -fdx to clean the source tree."
	@echo "If you run build.sh or make it will rebuild anyways."

check:
	./build.sh
	${MAKE} -Ctests
