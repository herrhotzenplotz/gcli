#!/usr/bin/env perl
#
use Test2::V0;

use Cwd qw(realpath getcwd);
use File::Basename;
use Data::Dumper;

BEGIN { push(@INC, realpath(dirname($0))); }
use server;

##################################################
# Parse in the test
sub parsetest {
	my ($filename) = @_;
	my %testprops = ();

	my ($in_multiline, $multi_key, $multi_val) = (
		0,
		undef,
		undef
	);

	open my $infile, '<', $filename or die "bad input file";
	while (my $ln = <$infile>) {
#		print "$ln";

		if ($ln eq "\n") {
			if ($in_multiline) {
				$testprops{$multi_key} = $multi_val;
				$in_multiline = 0;
			}

			next;
		}

		if ($in_multiline) {
			$multi_val = $multi_val . substr($ln, 2);
		} else {
			my $colon = index($ln, ':');
			my $key = substr($ln, 0, $colon);

			# detect multiline stuff
			if (length($ln) == $colon + 2) {
				$in_multiline = 1;
				$multi_key = $key;
				$multi_val = "";
				next;
			}

			$ln =~ s/\n$//;
			$testprops{$key} = substr($ln, $colon + 2);
		}
	}

	$infile->close();

	return %testprops;
}

my %testprops = parsetest $ARGV[0];

print Dumper(%testprops);

# Build the actual response body
my $bdy = $testprops{'ServerResponseBody'};
$bdy =~ s/\n$/\r\n/;
my $rsp = sprintf("HTTP/1.1 %s\r\n\r\n%s", $testprops{'ServerResponseStatus'}, $bdy);

my $srv = server::new(
	id => 2,
	responses => [ $rsp ]
);

my %results = $srv->run_gcli($testprops{'ClientArgs'});

# Client Verification
my @client_verify = (
	'VerifyClientExitCode',
	'VerifyClientOutput',
);

foreach (@client_verify) {
	is($results{$_}, $testprops{$_}, "Client check: $_") if defined($testprops{$_});
}

# Server Verification
my @server_verify = (
	'VerifyRequestMethod',
	'VerifyRequestPath',
	'VerifyRequestVersion',
	'VerifyRequestHeaders',
	'VerifyRequestBody',
);

my $rq = $srv->get_request;

foreach (@server_verify) {
	is($rq->{$_}, $testprops{$_}, "Server check: $_") if defined($testprops{$_});
}

$srv->kill;

done_testing;
