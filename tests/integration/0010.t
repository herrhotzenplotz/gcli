Keywords: github issue
ID: 10
Title: GitHub issue list with multiple pages

ClientArgs: -t github issues
VerifyClientExitCode: 0
VerifyClientOutput:
  NUMBER  NOTES  STATE  TITLE
       2      0  open   open issue 1
       1     42  open   open issue 2

# First request
ServerResponseStatus: 200 OK
ServerResponseHeaders:
  link: "@SERVERURL@/this_is_important"; rel="next"

ServerResponseBody:
  [ { "number": 2, "state": "open", "title": "open issue 1", "comments": 0  } ]

VerifyRequestMethod: GET
VerifyRequestPath: /repos/herrhotzenplotz/gcli/issues?state=open

# Second request
ServerResponseStatus: 200 OK
ServerResponseHeaders:

ServerResponseBody:
  [ { "number": 1, "state": "open", "title": "open issue 2", "comments": 42 } ]

VerifyRequestMethod: GET
VerifyRequestPath: /this_is_important
