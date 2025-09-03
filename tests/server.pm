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
	kill
	rungcli
);

use IO::Socket qw(AF_INET SOCK_STREAM IPPROTO_TCP);
use Socket qw(unpack_sockaddr_in pack_sockaddr_in inet_aton inet_ntoa);
use IO::File;
use Cwd;

sub new {
	my %parms = @_;
	my $cfg_file_name = "gcli_" . $parms{'id'} . ".conf";
	my $srv_out_file = "gcli_" . $parms{'id'} . ".srv.out";

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
	my $config_file = IO::File->new($cfg_file_name, "w");

	$config_file->print(<<EOF);
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
	$config_file->flush();
	$config_file->close();

	my $pid = fork;
	if ($pid != 0 ) {
		return bless {
			pid => $pid,
			lsock => $lsock,
			cfg_file_name => $cfg_file_name,
			srv_out_file => $srv_out_file,
		};
	}

	my $srv_out = IO::File->new($srv_out_file, "w");

	#################################################################
	# event loop
	for (;;) {
		my $client_sock = $lsock->accept();
		my ($client_port, $client_ip) = unpack_sockaddr_in($client_sock->peername());
		my $client_ip_str = inet_ntoa($client_ip);

		# Request
		while (my $line = $client_sock->getline()) {
			$line =~ s/\r\n$//;
			print $srv_out "$line\n";

			last if length($line) == 0;
		}

		$srv_out->flush();

		# Response, 404 for now
		$client_sock->printf('HTTP/1.1 404 Not Found\r\n\r\n{ "message": "not found" }\r\n');

		$client_sock->close();
	}

	exit 0;
}

sub kill {
	my ($self) = @_;

	kill('INT', $self->{'pid'});
	waitpid $self->{'pid'}, 0;

	$self->{'lsock'}->close();
	unlink $self->{'cfg_file_name'};
}

sub rungcli {
	my ($srv, $args) = @_;

	my $cwd = getcwd();
	my $cmd = "$cwd/gcli -C $srv->{'cfg_file_name'} $args 2>foo";

	my $output = qx($cmd);
	my $rc = $? >> 8; # see perldoc perlop

	return (rc => $rc, output => $output);
}

1;
