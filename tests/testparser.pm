package testparser;

our @EXPORT = (
	qw(parsetest)
);

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

			chomp $ln;
			$testprops{$key} = substr($ln, $colon + 2);
		}
	}

	$infile->close();

	return %testprops;
}

1;
