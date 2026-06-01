Keywords: regress pulls
ID: 25
Title: Regress: pulls -i id overflow

ClientArgs: -t github pulls -i -1 all
VerifyClientExitCode: 1
VerifyClientOutput:
  gcli: error: cannot parse pr number: Invalid argument

