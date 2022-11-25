#!/bin/sh -xe

fail()
{
    printf "fail: %s\n" "$*" >&2
    exit 1
}

diehard()
{
    printf "hard error: %s\n" "$*" >&2
    exit 99
}

PGEN=./pgen

[ -x $PGEN ] || diehard "pgen not found or executable"
