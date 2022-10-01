.PHONY: all clean check install
all:
	./build.sh

clean:
	@echo "Use git clean -fdx to clean the source tree."
	@echo "If you run build.sh or make it will rebuild anyways."

check:
	./build.sh check

install:
	PREFIX="${PREFIX}" DESTDIR="${DESTDIR}" ./build.sh install
