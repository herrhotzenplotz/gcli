Keywords: regress issue
ID: 24
Title: Regress: issue -i id overflow

# This is not specific to bugzilla
ClientArgs: -t bugzilla issues -i -1 all
VerifyClientExitCode: 1
VerifyClientOutput:
  gcli: error: cannot parse issue number: Invalid argument

