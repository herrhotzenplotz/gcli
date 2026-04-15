Keywords: gitlab issue milestone
ID: 16
Title: GitLab Issue Milestone

ClientArgs: -t gitlab issues -o foo -r bar -i 42 milestone 123456
VerifyClientExitCode: 0

ServerResponseStatus: 200 OK

ServerResponseBody:
  []

VerifyRequestMethod: PUT
VerifyRequestPath: /projects/foo%2Fbar/issues/42?milestone_id=123456
