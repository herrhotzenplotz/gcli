#!/usr/bin/env perl
#
# This file is part of gcli.
#
# Copyright Nico Sonack <nsonack@herrhotzenplotz.de>
package server;

use strict;
use warnings;

our @EXPORT = qw(
	runserver
);

use IO::Socket qw(AF_INET SOCK_STREAM IPPROTO_TCP);
use Socket qw(unpack_sockaddr_in pack_sockaddr_in inet_aton inet_ntoa);
use IO::File;

sub runserver {
	##################################################################
	# Open Socket
	my $lsock = IO::Socket->new(
		Domain => AF_INET,
		Type => SOCK_STREAM,
		Proto => IPPROTO_TCP,
	);

	$lsock->bind(pack_sockaddr_in(0, inet_aton("127.0.0.1"))) or die "bind";
	$lsock->listen(5);

	my ($port, $ip) = unpack_sockaddr_in($lsock->sockname());
	my $str_ip = inet_ntoa($ip);

	##################################################################
	# Write config file
	my $config_file = IO::File->new("gcli.conf", "w");

	print $config_file <<EOF;
	defaults {
		gitlab-default-account = gitlab
		github-default-account = github
		gitea-default-account = gitea
		bugzilla-default-account = bugzilla
	}

	gitlab {
		forge-type = gitlab
		token = foobar
		account = tester
		apibase = http://${str_ip}:${port}
	}

	gitea {
		forge-type = gitea
		token = foobar
		account = tester
		apibase = http://${str_ip}:${port}
	}

	bugzilla {
		forge-type = bugzilla
		token = foobar
		account = tester
		apibase = http://${str_ip}:${port}
	}

	github {
		forge-type = github
		token = foobar
		account = tester
		apibase = http://${str_ip}:${port}
	}
EOF
	$config_file->close();

	#################################################################
	# event loop
	for (;;) {
		my $client_sock = $lsock->accept();
		my ($client_port, $client_ip) = unpack_sockaddr_in($client_sock->peername());
		my $client_ip_str = inet_ntoa($client_ip);

		print STDERR "server.pl: got a connection from http://${client_ip_str}:${client_port}/\n";

		# Request
		while (my $line = $client_sock->getline()) {
			$line =~ s/\r\n$//;
			print STDERR "client > $line\n";

			last if length($line) == 0;
		}

		# Response, 404 for now
		$client_sock->printf("HTTP/1.1 404 Not Found\r\n\r\nNot Found\r\n");

		$client_sock->close();
	}
}

1;
