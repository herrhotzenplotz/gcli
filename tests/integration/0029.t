Keywords: regress unicode table output
ID: 29
Title: Regress: Misaligned table with emojis

ClientArgs: -a gitea labels -o foo -r bar
VerifyClientExitCode: 0
VerifyClientOutput:
  ID  NAME                                   DESCRIPTION
  33  Malmö 🚀                               Banana
  34  Priority::ShouldHaveBeenDoneYesterday  Something

ServerResponseStatus: 200 OK
ServerResponseBody:
  [
    {
      "id": 33,
      "name": "Malmö 🚀",
      "exclusive": false,
      "is_archived": false,
      "color": "808080",
      "description": "Banana",
      "url": "http://gitea/api/v1/repos/foo/bar/labels/33"
    },
    {
      "id": 34,
      "name": "Priority::ShouldHaveBeenDoneYesterday",
      "exclusive": false,
      "is_archived": false,
      "color": "ffffff",
      "description": "Something",
      "url": "http://gitea/api/v1/repos/foo/bar/labels/34"
    }
  ]

# 2nd request is for the org labels, ignore
ServerResponseStatus: 404 Not found
ServerResponseBody: 404 Not found
