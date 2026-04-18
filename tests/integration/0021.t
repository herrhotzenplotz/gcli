Keywords: bugzilla issue
ID: 21
Title: Bugzilla single bug fetch using -a flag with -i

ClientArgs: -a bugzilla issues -i 22 all
VerifyClientExitCode: 0
VerifyClientOutput:
     NUMBER : 22
      TITLE : Test bug
    CREATED : 2020-Jan-01 00:00:00
    PRODUCT : Test Product
  COMPONENT : General
     AUTHOR : Test User
      STATE : Open
     LABELS : none
  ASSIGNEES : none
  
  ORIGINAL POST
  
  
      This is the test bug description.
  

# First request: fetch the bug by ID
ServerResponseStatus: 200 OK
ServerResponseBody:
  {
    "bugs": [
      {
        "id": 22,
        "summary": "Test bug",
        "status": "Open",
        "creation_time": "2020-01-01T00:00:00Z",
        "creator_detail": { "real_name": "Test User" },
        "product": "Test Product",
        "component": "General"
      }
    ]
  }

VerifyRequestMethod: GET
VerifyRequestPath: /rest/bug?limit=1&id=22

# Second request: fetch the original post
ServerResponseStatus: 200 OK
ServerResponseBody:
  {
    "bugs": {
      "22": {
        "comments": [
          { "text": "This is the test bug description.", "creator": "tester" }
        ]
      }
    }
  }

VerifyRequestMethod: GET
VerifyRequestPath: /rest/bug/22/comment?include_fields=_all
