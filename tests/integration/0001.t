Keywords: basic
Title: basic sanity check. if this doesn't run the build and gcli are generally broken.

ClientArgs: -t github issues -o herrhotzenplotz -r gcli
VerifyClientExitCode: 0
VerifyClientOutput:
  No issues

ServerResponseStatus: 200 OK
ServerResponseBody:
  []

VerifyRequestMethod: GET
VerifyRequestPath: /repos/herrhotzenplotz/gcli/issues?state=open
