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

my $builddir = getcwd();

my $output = `${builddir}/gcli version 2>&1`;
ok($! == 0);

done_testing;
