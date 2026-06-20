Keywords: bugzilla attachments
ID: 27
Title: Bugzilla attachment not found

ClientArgs: -a bugzilla attachments -i 1 get
VerifyClientExitCode: 1
VerifyClientOutput:
  gcli: failed to get attachment: no attachment in result

# First request: fetch the bug by ID
ServerResponseStatus: 200 OK
ServerResponseBody:
  {"attachments":{},"bugs":{}}

VerifyRequestMethod: GET
VerifyRequestPath: /rest/bug/attachment/1

