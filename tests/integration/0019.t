Keywords: gitlab issue
ID: 19
Title: GitLab issue list using gl: prefix with nested group path

ClientArgs: issues gl:foo/bar/baz/my-project
VerifyClientExitCode: 0
VerifyClientOutput:
  NUMBER  NOTES  STATE   TITLE
       1      0  opened  nested group issue

ServerResponseStatus: 200 OK
ServerResponseBody:
  [
    { "iid": 1, "state": "opened", "title": "nested group issue", "user_notes_count": 0 }
  ]

VerifyRequestMethod: GET
VerifyRequestPath: /projects/foo%2Fbar%2Fbaz%2Fmy-project/issues?state=opened
