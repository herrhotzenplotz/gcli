#!/usr/bin/env perl
#
# This is the test runner for integration tests.
# It reads a given file and based on it takes actions.
#
# This file is part of gcli.
use Test2::V0;

use Cwd qw(realpath getcwd);
use File::Basename;
use Data::Dumper;

BEGIN { push(@INC, realpath(dirname($0))); }
use server;
use testparser;

# Get a value from the test props
if ($ARGV[0] eq "-g") {
	my %testprops = testparser::parsetest $ARGV[2];

	if (!defined($testprops{$ARGV[1]})) {
		die(sprintf "%s not defined by test %s", $ARGV[1], $ARGV[2]);
	}

	print $testprops{$ARGV[1]};
	exit 0;
}

# Otherwise just continue running the test
my %testprops = testparser::parsetest $ARGV[0];

# Build the actual response body
my @responses = ();

if (scalar($testprops{'ServerResponseBody'}) eq "ARRAY") {
} else {
	my $bdy = $testprops{'ServerResponseBody'};
	$bdy =~ s/\n$/\r\n/;
	my $rsp = sprintf("HTTP/1.1 %s\r\n\r\n%s", $testprops{'ServerResponseStatus'}, $bdy);

	push(@responses, $rsp);
}

my $srv = server::new(
	id => $testprops{'ID'},
	responses => \@responses,
);

my %results = $srv->run_gcli($testprops{'ClientArgs'});

# Client Verification
my @client_verify = (
	'VerifyClientExitCode',
	'VerifyClientOutput',
);

foreach (@client_verify) {
	if (defined($testprops{$_})) {
		ok(defined($results{$_}), "Client check: $_ not undefined");
		is($results{$_}, $testprops{$_}, "Client check: $_");
	}
}

# Server Verification
my @server_verify = (
	'VerifyRequestMethod',
	'VerifyRequestPath',
	'VerifyRequestVersion',
	'VerifyRequestHeaders',
	'VerifyRequestBody',
);

for (my $i = 0; $i < $#responses; ++$i) {
	my $rq = $srv->get_request($i);

	foreach (@server_verify) {
		if (defined($testprops{$_})) {
			ok(defined($rq->{$_}), "Server check: $_ not undefined");
			is($rq->{$_}, $testprops{$_}, "Server check: $_");
		}
	}
}

$srv->kill;

done_testing;
