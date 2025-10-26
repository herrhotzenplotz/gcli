Keywords: gitea issue
ID: 7
Title: Gitea issue list

ClientArgs: -t gitea issues -o herrhotzenplotz -r gcli
VerifyClientExitCode: 0
VerifyClientOutput:
  NUMBER  NOTES  STATE  TITLE
       2      0  open   open issue 1
       1     42  open   open issue 2

ServerResponseStatus: 200 OK
ServerResponseBody:
  [
    { "number": 2, "state": "open", "title": "open issue 1", "comments": 0  },
    { "number": 1, "state": "open", "title": "open issue 2", "comments": 42 }
  ]

VerifyRequestMethod: GET
VerifyRequestPath: /repos/herrhotzenplotz/gcli/issues?type=issues&state=open
