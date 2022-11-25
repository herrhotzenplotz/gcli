#!/bin/sh -xe

. $(dirname $0)/setup.sh

TMPFILE=$(mktemp)
cat <<EOF | ! ${PGEN} > $TMPFILE || fail "parse suceeded"
parser github_issue
is object of gcli_issue
with
   ();
EOF

rm -f $TMPFILE
