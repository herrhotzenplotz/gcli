Keywords: bugzilla issue
ID: 5
Title: Bugzilla Bug List

ClientArgs: -t bugzilla issues
VerifyClientExitCode: 0
VerifyClientOutput:
  NUMBER  NOTES  STATE  TITLE
      22      0  Open   This issue is open
      23      0  New    This issue is new

ServerResponseStatus: 200 OK
ServerResponseBody:
  {
    "bugs": [
      { "id": 22, "summary": "This issue is open", "status": "Open" },
      { "id": 23, "summary": "This issue is new", "status": "New"  }
    ]
  }

VerifyRequestMethod: GET
VerifyRequestPath: /rest/bug?order=bug_id%20DESC%2C&limit=30&status=Open&status=New
