Keywords: github issue
ID: 2
Title: GitHub issue list

ClientArgs: -t github issues -o herrhotzenplotz -r gcli
VerifyClientExitCode: 0
VerifyClientOutput:
  NUMBER  NOTES  STATE  TITLE
      42     69  open   testing hello

ServerResponseStatus: 200 OK
ServerResponseBody:
  [ { "number": 42, "state": "open", "title": "testing hello", "comments": 69 } ]

VerifyRequestMethod: GET
VerifyRequestPath: /repos/herrhotzenplotz/gcli/issues?state=open
