#!/bin/sh
# Set the following options:
# -e: Exit immediately if any command exits with a non-zero status (error).
# -u: Treat unset variables as errors, causing the script to exit.
set -eu

#
# Static Site generator for the tutorial.
#
#  You will need cmark for this to work.
#

header() {
    title="${1}"
    sed "s/{{TITLE_PLACEHOLDER}}/${title}/g" top.html
}

footer() {
    cat footer.html
}

genindex() {
    header "Index"

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

    header "${PAGETITLE}"

    echo "<p><a href=\"index.html\">⇐ Back to table of contents</a></p>"
    echo "<hr />"
    cmark -t html < ${PAGEMDFILE}
    echo "<br />"
    echo "<hr />"
    echo "<p><a href=\"index.html\">⇐ Back to table of contents</a></p>"

    footer
}

genindex > index.html

while IFS="$(printf '\t')" read -r htmldoc title; do
    mddoc="${htmldoc%.html}.md"

    genpage "${title}" "${mddoc}" > "${htmldoc}"
done < toc
