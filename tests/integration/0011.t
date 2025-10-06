Keywords: gitlab issue
ID: 11
Title: GitLab issue list with multiple pages

ClientArgs: -t gitlab issues
VerifyClientExitCode: 0
VerifyClientOutput:
  NUMBER  NOTES  STATE   TITLE
       2      0  opened  open issue 1
       1     42  opened  open issue 2

# First request
ServerResponseStatus: 200 OK
ServerResponseHeaders:
  link: "@SERVERURL@/this_is_important"; rel="next"

ServerResponseBody:
  [ { "iid": 2, "state": "opened", "title": "open issue 1", "user_notes_count": 0  } ]

VerifyRequestMethod: GET
VerifyRequestPath: /projects/herrhotzenplotz%2Fgcli/issues?state=opened

# Second request
ServerResponseStatus: 200 OK
ServerResponseHeaders:

ServerResponseBody:
  [ { "iid": 1, "state": "opened", "title": "open issue 2", "user_notes_count": 42 } ]

VerifyRequestMethod: GET
VerifyRequestPath: /this_is_important
