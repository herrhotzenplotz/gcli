#!/bin/sh

#
# Static Site generator for the tutorial.
#
#  You will need cmark for this to work.
#

header() {
    cat top.html
    printf "<title>%s</title>\n" "${1}"
    cat middle.html
}

footer() {
    cat footer.html
}

genindex() {
    header "GCLI Tutorial"

    cat <<EOF
    <h1>A GCLI Tutorial</h1>
    <p>This document is aimed at those who are new to gcli and want get
    started using it.</p>

    <h2>Table of contents</h2>
EOF

    echo "<ol>"

    awk -F\\t '{printf "<li><a href=\"./%s\">%s</a></li>\n", $1, $2}' < toc

    echo "</ol>"

    footer
}

genpage() {
    PAGETITLE="${1}"
    PAGEMDFILE="${2}"

    header "GCLI Tutorial | ${PAGETITLE}"

    echo "<p><a href=\"index.html\">⇐ Back to table of contents</a></p>"
    echo "<hr />"
    cmark -t html < ${PAGEMDFILE}
    echo "<br />"
    echo "<hr />"
    echo "<p><a href=\"index.html\">⇐ Back to table of contents</a></p>"

    footer
}

genindex > index.html

while read record; do
    title="$(echo ${record} | awk '{print $2}')"
    htmldoc="$(echo ${record} | awk '{print $1}')"
    mddoc="${htmldoc%.html}.md"

    genpage "${title}" "${mddoc}" > "${htmldoc}"
done < toc
