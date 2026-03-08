Keywords: gitea issue
ID: 14
Title: Gitea Issue Due Date

ClientArgs: -v -t gitea issues -o foo -r bar -i 42 due 2038-01-20
VerifyClientExitCode: 0

ServerResponseStatus: 200 OK

ServerResponseBody:
  []

VerifyRequestMethod: PATCH
VerifyRequestPath: /repos/foo/bar/issues/42
VerifyRequestBody: {"due_date": "2038-01-20T00:00:00Z"}
