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
	my $srv_log_file = "gcli_" . $parms{'id'} . ".srv.log";
	my $gcli_out_file = "gcli_" . $parms{'id'} . ".console.log";

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
	disable-spinner = true
	render-markdown = false
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
			gcli_out_file => $gcli_out_file,
		};
	}

	open my $srv_log, '>', $srv_log_file or die "cannot open server log file";
	$srv_log->autoflush;

	#################################################################
	# event loop
	my $max_reqs = scalar(@{$parms{'responses'}});
	printf $srv_log "Waiting for %d requests\n", $max_reqs;

	for (my $i = 0; $i < $max_reqs; ++$i) {
		my $client_sock = $lsock->accept();
		my ($client_port, $client_ip) = unpack_sockaddr_in($client_sock->peername());
		my $client_ip_str = inet_ntoa($client_ip);
		my $srv_out = IO::File->new($srv_out_file . ".${i}", "w");
		my $rsp = $parms{'responses'}[$i];
		my $content_length = -1;

		print $srv_log "Received connection from ${client_ip_str}\n";

		# Request
		while (my $line = $client_sock->getline()) {
			$line =~ s/\r\n$//;
			print $srv_out "$line\n";
			print $srv_log "  > $line\n";

			# Read a content length, if it exists
			my ($k, $v) = split /:\s*/, $line;
			if ($k and ($k eq "Content-Length")) {
				$content_length = $v + 0;
			}

			last if length($line) == 0;
		}

		# Handle the body
		if ($content_length > 0) {
			print $srv_log "Content length = $content_length\n";
			print $srv_log "Reading this many bytes!\n";

			my $body;
			$client_sock->read($body, $content_length);

			print $srv_out $body;
			print $srv_log " > Body:\n$body\n";
		}

		$srv_out->flush();
		$srv_out->close();

		$rsp =~ s/\@SERVERURL\@/http:\/\/${str_ip}:${port}/g;

		print $srv_log "Response is as follows\n";
		print $srv_log $rsp;

		$client_sock->printf("%s", $rsp);
		$client_sock->close();
	}

	$srv_log->flush();
	$srv_log->close();

	exit 0;
}

sub kill {
	my ($self) = @_;

	kill('KILL', $self->{'pid'});
	waitpid $self->{'pid'}, 0;

	$self->{'lsock'}->close();
	unlink $self->{'cfg_file_name'};
}

sub run_gcli {
	my ($srv, $args) = @_;

	my $cwd = getcwd();
	my $cmd = "$cwd/gcli -C $srv->{'cfg_file_name'} $args 2>&1";

	open my $logfile, '>', $srv->{'gcli_out_file'};
	$logfile->autoflush;

	my $output = qx($cmd);
	my $rc = $? >> 8; # see perldoc perlop

	$logfile->print($output);
	$logfile->close();

	return (VerifyClientExitCode => $rc, VerifyClientOutput => $output);
}

sub get_request {
	my ($srv, $i) = @_;

	my $fname = $srv->{'srv_out_file'} . ".$i";

	# Open server output file
	return bless {} if not -e $fname;

	open my $f, '<', $fname;

	# Parse the request line
	my ($method, $path, $version) = split ' ', <$f>;

	# Parse headers
	my @headers = ();
	while (my $ln = <$f>) {
		last if ($ln eq "\r\n" or $ln eq "\n");

		my ($k, $v) = split ": ", $ln;
		push(@headers, { $k => $v });
	}

	# And the body
	my $body = "";
	while (my $ln = <$f>) {
		$body = $body . $ln;
	}

	return bless {
		VerifyRequestMethod => $method,
		VerifyRequestPath => $path,
		VerifyRequestVersion => $version,
		VerifyRequestHeaders => \@headers,
		VerifyRequestBody => $body,
	};
}

1;
