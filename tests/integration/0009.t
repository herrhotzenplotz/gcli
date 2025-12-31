Keywords: github issue
ID: 9
Title: GitHub issue list with -a

ClientArgs: -t github issues -a -o herrhotzenplotz -r gcli
VerifyClientExitCode: 0
VerifyClientOutput:
  NUMBER  NOTES  STATE   TITLE
       3      2  closed  this one is closed
       2      0  open    open issue 1
       1     42  open    open issue 2

ServerResponseStatus: 200 OK
ServerResponseBody:
  [
    { "number": 3, "state": "closed", "title": "this one is closed", "comments": 2  },
    { "number": 2, "state": "open",   "title": "open issue 1",       "comments": 0  },
    { "number": 1, "state": "open",   "title": "open issue 2",       "comments": 42 }
  ]

VerifyRequestMethod: GET
VerifyRequestPath: /repos/herrhotzenplotz/gcli/issues?state=all
