Keywords: regress github repo
ID: 28
Title: Regress: GitHub create repo without description

ClientArgs: -a github repo create -r banana
VerifyClientExitCode: 0

# First request: fetch the bug by ID
ServerResponseStatus: 200 OK
ServerResponseBody:
  {}

VerifyRequestMethod: POST
VerifyRequestPath: /user/repos
VerifyRequestBody:
  {"name": "banana", "private": false}
