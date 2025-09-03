#!/usr/bin/env perl
#
# KEYWORDS: basic
# TITLE: basic sanity check. if this doesn't run the build and gcli are generally broken.

use Test2::V0;

use Cwd qw(realpath getcwd);
use File::Basename;

BEGIN { push(@INC, realpath(dirname($0) . "/..")); }
use server;

my $builddir = getcwd();

server::runserver();

done_testing;
