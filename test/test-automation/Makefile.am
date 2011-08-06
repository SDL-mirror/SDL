ACLOCAL_AMFLAGS = -I acinclude -I build-scripts

SUBDIRS = src/libtest src/runner \
		  tests/testdummy \
		  tests/testrect \
		  tests/testplatform \
		  tests/testaudio \
		  tests/testsurface \
		  tests/testrwops \
		  tests/testrender

all-local: install-tests
install: install-tests
install-tests:
	$(SHELL) build-scripts/install-tests.sh

clean:
clean-local:
	echo "clean-local is obsolete"
