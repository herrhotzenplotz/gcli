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

use Cwd qw(getcwd realpath);
use File::Basename;
use Getopt::Long;
use TAP::Harness;

#####################
# Environment setup
my $testsrcdir = dirname(realpath($0));
my $builddir = getcwd();
my $verbosity = 0;
my $jobs = 1;

#########################
# Command line Options
sub usage {
	print STDERR "usage: $0 [-j <njobs>] [-s <srcdir>] [-b <builddir>] [-v]\n";
	print STDERR "OPTIONS:\n";
	print STDERR "   -j --jobs      Run tests in parallel with the given number of jobs\n";
	print STDERR "   -s --srcdir    Assume the test source tree is at the given directory\n";
	print STDERR "   -b --builddir  Assume the build output directory is at the given directory\n";
	print STDERR "   -v --verbose   Be verbose\n";
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

	push(@alltests, [ "tests/unit/$name", "U: $name" ]);
}

for (glob "${testsrcdir}/integration/*.t") {
	# Push the integration tests, the second argument here is the title
	push(@alltests, ["$_", "I: " . `${testsrcdir}/integrationwrap.pl -g Title $_`]);
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
$harness->runtests(@alltests);
