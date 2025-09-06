Keywords: basic
ID: 1
Title: Sanity Check

ClientArgs: -t github issues -o herrhotzenplotz -r gcli
VerifyClientExitCode: 0
VerifyClientOutput:
  No issues

ServerResponseStatus: 200 OK
ServerResponseBody:
  []

VerifyRequestMethod: GET
VerifyRequestPath: /repos/herrhotzenplotz/gcli/issues?state=open
