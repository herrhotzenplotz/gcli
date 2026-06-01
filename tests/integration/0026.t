Keywords: regress attachments bugzilla
ID: 26
Title: Regress: attachments -i id overflow

# This is not specific to bugzilla
ClientArgs: -t bugzilla attachments -i -1 get
VerifyClientExitCode: 1
VerifyClientOutput:
  gcli: error: cannot parse attachment id: Invalid argument

