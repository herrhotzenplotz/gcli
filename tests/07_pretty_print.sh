#!/bin/sh -xe

. $(dirname $0)/setup.sh

TMPFILE=$(mktemp)

[ -x ./tests/pretty_print_test ] || diehard "pretty_print_test executable found"

./tests/pretty_print_test > $TMPFILE \
    || fail "pretty_print failed"

diff -u $srcdir/tests/samples/pretty.dump $TMPFILE \
    || fail "Unexpected test output"

rm -f $TMPFILE
