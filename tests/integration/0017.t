Keywords: github issue
ID: 17
Title: GitHub issue list using gh: prefix

ClientArgs: issues gh:herrhotzenplotz/gcli
VerifyClientExitCode: 0
VerifyClientOutput:
  NUMBER  NOTES  STATE  TITLE
      42     69  open   testing hello

ServerResponseStatus: 200 OK
ServerResponseBody:
  [ { "number": 42, "state": "open", "title": "testing hello", "comments": 69 } ]

VerifyRequestMethod: GET
VerifyRequestPath: /repos/herrhotzenplotz/gcli/issues?state=open
