Keywords: bugzilla issue
ID: 6
Title: Bugzilla Bug List with -a

ClientArgs: -t bugzilla issues -a
VerifyClientExitCode: 0
VerifyClientOutput:
  NUMBER  NOTES  STATE   TITLE
      22      0  Open    This issue is open
      23      0  New     This issue is new
      24      0  Closed  This issue is closed

ServerResponseStatus: 200 OK
ServerResponseBody:
  {
    "bugs": [
      { "id": 22, "summary": "This issue is open",   "status": "Open"   },
      { "id": 23, "summary": "This issue is new",    "status": "New"    },
      { "id": 24, "summary": "This issue is closed", "status": "Closed" }
    ]
  }

VerifyRequestMethod: GET
VerifyRequestPath: /rest/bug?order=bug_id%20DESC%2C&limit=30&status=All
