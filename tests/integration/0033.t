Keywords: gitea repos
ID: 33
Title: Gitea delete repository

ClientArgs: -t gitea repos -o banana -r foobar delete -y
VerifyClientExitCode: 0

ServerResponseStatus: 200 OK

VerifyRequestMethod: DELETE
VerifyRequestPath: /repos/banana/foobar
