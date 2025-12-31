#!/usr/bin/env perl
#
# This is the main test harness for the gcli test suite.
# Its job is to look for tests and run them.
# Afterwards it should summarise the results.
#
# This file is part of gcli.
#
# Copyright Nico Sonack <nsonack@herrhotzenplotz.de>

use warnings;
use strict;

# Minimum required perl version
use 5.006002;

use Cwd qw(getcwd realpath);
use File::Basename;
use Getopt::Long;
use TAP::Harness;

BEGIN { push(@INC, dirname(realpath($0))); }
use testparser;

#####################
# Environment setup
my $testsrcdir = dirname(realpath($0));
my $builddir = getcwd();
my $verbosity = 0;
my $jobs = 1;

# Clean environment variables that could mess with the
# test suite results.
$ENV{'TZ'} = "UTC";
$ENV{'LC_ALL'} = "C.UTF-8";
delete $ENV{'GCLI_ACCOUNT'};

#########################
# Command line Options
sub usage {
	print STDERR "usage: $0 [-j <njobs>] [-s <srcdir>] [-b <builddir>] [-v] [filter]\n";
	print STDERR "OPTIONS:\n";
	print STDERR "   -j --jobs      Run tests in parallel with the given number of jobs\n";
	print STDERR "   -s --srcdir    Assume the test source tree is at the given directory\n";
	print STDERR "   -b --builddir  Assume the build output directory is at the given directory\n";
	print STDERR "   -v --verbose   Be verbose\n";
	print STDERR "\n";
	print STDERR "When a filter is given, only integration tests are run.\n";
	print STDERR "A filter is either a keyword or a test ID.\n";
}

GetOptions(
	"srcdir=s" => \$testsrcdir,
	"builddir=s" => \$builddir,
	"verbose" => \$verbosity,
	"jobs=i" => \$jobs,
	"help" => sub { usage; exit 0; },
) or die "failed to parse command line arguments";

chdir $builddir;

##########################
# Look for unit tests
my @alltests = ();

while (glob("${testsrcdir}/unit/*.c")) {
	my $name = fileparse($_, ".c");

	push(@alltests, [ "tests/unit/$name", "U: $name" ]) if ($#ARGV != 0);
}

for (glob "${testsrcdir}/integration/*.t") {
	# Push the integration tests, the second argument here is the title
	my %data = testparser::parsetest $_;
	my @kws = split / /, $data{Keywords};

	my @tdescr = ["$_", "I: #${data{ID}}: ${data{Title}}"];

	if (scalar(@ARGV) != 0) {
		foreach my $arg (@ARGV) {
			# test id
			if ($data{ID} eq $arg) {
				push(@alltests, @tdescr);
				last;
			}

			# test keywords
			if (scalar(grep($arg eq $_, @kws)) != 0) {
				push(@alltests, @tdescr);
				last;
			}
		}
	} else {
		push(@alltests, @tdescr);
	}
}

my $harness = TAP::Harness->new({
	verbosity => $verbosity,
	failures => 1,
	timer => 1,
	show_count => 1,
	errors => 1,
	jobs => $jobs,
	exec => sub {
		my ($harness, $test_file) = @_;

		return [ "${testsrcdir}/integrationwrap.pl", $test_file ] if $test_file =~ /\.t$/;
		return undef;
	},
});

my $agg = $harness->runtests(@alltests);
exit 1 if $agg->has_errors;

# kak: filetype=perl
