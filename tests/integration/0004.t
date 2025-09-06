Keywords: gitlab issue
ID: 4
Title: GitLab issue list with -a

ClientArgs: -t gitlab issues -o herrhotzenplotz -r gcli -a
VerifyClientExitCode: 0
VerifyClientOutput:
  NUMBER  NOTES  STATE   TITLE
      22      1  closed  this one is closed
      42     69  opened  testing hello
      43      0  opened  hello world!

ServerResponseStatus: 200 OK
ServerResponseBody:
  [
    { "iid": 22, "state": "closed", "title": "this one is closed", "user_notes_count": 1 },
    { "iid": 42, "state": "opened", "title": "testing hello", "user_notes_count": 69 },
    { "iid": 43, "state": "opened", "title": "hello world!",  "user_notes_count": 0  }
  ]

VerifyRequestMethod: GET
VerifyRequestPath: /projects/herrhotzenplotz%2Fgcli/issues
