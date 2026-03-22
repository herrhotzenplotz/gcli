Keywords: gitlab issue create
ID: 15
Title: Create a GitLab issue noninteractively

ClientArgs: -v -t gitlab issues create -o foo -r bar -y -T /dev/null 'TEST'
VerifyClientExitCode: 0

ServerResponseStatus: 200 OK

ServerResponseBody:
  []

VerifyRequestMethod: POST
VerifyRequestPath: /projects/foo%2Fbar/issues
VerifyRequestBody: {"title": "TEST", "description": ""}
