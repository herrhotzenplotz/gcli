#!/usr/bin/env perl
#
# This is the test runner for integration tests.
# It reads a given file and based on it takes actions.
#
# This file is part of gcli.
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

sub build_response {
	my ($status, $headers, $body) = @_;

	$status = '200 OK' if !defined($status);
	$headers = '' if !defined($headers);

	$body =~ s/\n$/\r\n/;
	$headers =~ s/\n$/\r\n/;

	return sprintf("HTTP/1.1 %s\r\n%s\r\n%s", $status, $headers, $body);
}

if (ref($testprops{'ServerResponseBody'}) eq "ARRAY") {
	my $n_reqs = scalar(@{$testprops{'ServerResponseBody'}});
	for (my $i = 0; $i < $n_reqs; ++$i) {
		my $status = $testprops{'ServerResponseStatus'}[$i];
		my $headers = $testprops{'ServerResponseHeaders'}[$i];
		my $body = $testprops{'ServerResponseBody'}[$i];
		my $rsp = build_response($status, $headers, $body);

		push(@responses, $rsp);
	}
} else {
	my $status = $testprops{'ServerResponseStatus'};
	my $headers = $testprops{'ServerResponseHeaders'};
	my $body = $testprops{'ServerResponseBody'};
	my $rsp = build_response($status, $headers, $body);

	push(@responses, $rsp);
}

my $srv = server::new(
	id => $testprops{'ID'},
	responses => \@responses,
);

my %results = $srv->run_gcli($testprops{'ClientArgs'});

my $check_id = 1;

sub ok {
	my ($result, $description) = @_;

	if ($result) {
		printf "ok %d - %s\n", $check_id++, $description;
	} else {
		printf "not ok %d - %s\n", $check_id++, $description;
	}
}

sub is {
	my ($have, $want, $description) = @_;

	if ($have eq $want) {
		printf "ok %d - %s\n", $check_id++, $description;
	} else {
		# Hacky way to eliminate newlines
		$have =~ s/\n/\\n/g;
		$want =~ s/\n/\\n/g;

		printf "not ok %d - %s\n", $check_id++, $description;
		printf "  ---\n";
		printf "  data:\n";
		printf "    got: \"%s\"\n", $have;
		printf "    expect: \"%s\"\n", $want;
		printf "  ...\n";
	}
}

print "TAP version 14\n";

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
		if (defined($testprops{$_}[$i])) {
			ok(defined($rq->{$_}), "Server check: $_ not undefined");
			is($rq->{$_}, $testprops{$_}[$i], "Server check: $_");
		}
	}
}

$srv->kill;

printf "1..%d\n", $check_id - 1;

# kak: filetype=perl
