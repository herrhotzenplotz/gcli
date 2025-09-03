#!/usr/bin/env perl
#
# KEYWORDS: basic
# TITLE: basic sanity check. if this doesn't run the build and gcli are generally broken.

use Test2::V0;
use strict;
use warnings;

use Cwd qw(realpath getcwd);
use File::Basename;

BEGIN { push(@INC, realpath(dirname($0) . "/..")); }
use server;

my $srv = server::new(id => 2);

my %results = $srv->rungcli("-t github issues -o herrhotzenplotz -r gcli");

ok($results{'rc'} == 1, "exit code must be 1");

$srv->kill;

done_testing;
