#!/usr/bin/env perl
#
# KEYWORDS: basic
# TITLE: basic sanity check. if this doesn't run the build and gcli are generally broken.

use Test2::V0;

use Cwd qw(realpath getcwd);
use File::Basename;

BEGIN { push(@INC, realpath(dirname($0) . "/..")); }
use server;

my $srv = server::new(
	id => 2,
	responses => [
		"HTTP/1.1 200 OK\r\n" .
		"\r\n" .
		"[]\r\n"
	]
);

my %results = $srv->run_gcli("-t github issues -o herrhotzenplotz -r gcli");

is($results{'rc'}, 0, "exit code must be 1");
is($results{'output'}, "No issues\n", "output should indicate no issues");

my $rq = $srv->get_request;

is($rq->{'method'}, "GET", "Expected a GET request");
is($rq->{'path'}, "/repos/herrhotzenplotz/gcli/issues?state=open");

$srv->kill;

done_testing;
