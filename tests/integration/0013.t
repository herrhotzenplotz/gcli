Keywords: gitlab issue
ID: 13
Title: GitLab Issue Due Date

ClientArgs: -t gitlab issues -o foo -r bar -i 42 due 2038-01-20
VerifyClientExitCode: 0

ServerResponseStatus: 200 OK

ServerResponseBody:
  []

VerifyRequestMethod: PUT
VerifyRequestPath: /projects/foo%2Fbar/issues/42?due_date=2038-01-20
