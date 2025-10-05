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
		next if $ln =~ /^\#/;  # comments

		if ($ln eq "\n") {
			if ($in_multiline) {
				if (defined($testprops{$multi_key})) {
					if (scalar($testprops{$multi_key}) eq "ARRAY") {
						push(@{$testprops{$multi_key}}, $multi_val);
					} else {
						$testprops{$multi_key} = [$testprops{$multi_key}, $multi_val];
					}
				} else {
					$testprops{$multi_key} = $multi_val;
				}

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

			my $val = substr($ln, $colon + 2);
			if (defined($testprops{$key})) {
				if (scalar($testprops{$key}) eq "ARRAY") {
					push(@{$testprops{$key}}, $val);
				} else {
					$testprops{$key} = [$testprops{$key}, $val];
				}
			} else {
				$testprops{$key} = $val;
			}
		}
	}

	$infile->close();

	return %testprops;
}

1;
