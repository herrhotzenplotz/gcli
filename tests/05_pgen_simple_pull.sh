#!/bin/sh -xe

. $(dirname $0)/setup.sh

TMPFILE=$(mktemp)

[ -x ./tests/pgen-tests ] || diehard "pgen-tests executable found"

./tests/pgen-tests pulls < $srcdir/tests/samples/github_simple_pull.json > $TMPFILE \
    || fail "Parse Step failed"

diff -u $srcdir/tests/samples/parse_simple_pull.dump $TMPFILE \
    || fail "Unexpected output of parser"

rm -f $TMPFILE
