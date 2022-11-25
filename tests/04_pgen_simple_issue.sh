#!/bin/sh -xe

. $(dirname $0)/setup.sh

TMPFILE=$(mktemp)

[ -x ./tests/pgen-tests ] || diehard "pgen-tests executable found"

./tests/pgen-tests issues < $srcdir/tests/samples/github_simple_issue.json > $TMPFILE \
    || fail "Parse Step failed"

diff -u $srcdir/tests/samples/parse_simple_issue.dump $TMPFILE \
    || fail "Unexpected output of parser"

rm -f $TMPFILE
