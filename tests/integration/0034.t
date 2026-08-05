Keywords: gitea repos
ID: 34
Title: Gitea change visibility of repository

ClientArgs: -t gitea repos -o banana -r foobar set-visibility private
VerifyClientExitCode: 0

ServerResponseStatus: 200 OK

VerifyRequestMethod: PATCH
VerifyRequestPath: /repos/banana/foobar
VerifyRequestBody: { "private": true }
