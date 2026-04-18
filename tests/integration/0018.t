Keywords: gitlab issue
ID: 18
Title: GitLab issue list using gl: prefix

ClientArgs: issues gl:herrhotzenplotz/gcli
VerifyClientExitCode: 0
VerifyClientOutput:
  NUMBER  NOTES  STATE   TITLE
      42     69  opened  testing hello
      43      0  opened  hello world!

ServerResponseStatus: 200 OK
ServerResponseBody:
  [
    { "iid": 42, "state": "opened", "title": "testing hello", "user_notes_count": 69 },
    { "iid": 43, "state": "opened", "title": "hello world!",  "user_notes_count": 0  }
  ]

VerifyRequestMethod: GET
VerifyRequestPath: /projects/herrhotzenplotz%2Fgcli/issues?state=opened
