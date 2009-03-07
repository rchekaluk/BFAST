#!/usr/bin/perl -w

# Author: Nils Homer

use strict;
use warnings;
use Getopt::Std;

select STDERR; $| = 1;  # make unbuffered

my %opts;
my $version = '0.1.1';
my $usage = qq{
Usage: solid2fastq.pl <list of .csfasta files> <list of .qual files>

Note: 	The list of .csfasta files should be in the same order as the
		list of .qual files.  For example, if we have two .csfasta 
		files named <prefix>F3.csfasta and <prefix>R3.csfasat with their
		respecitive .qual files named <prefix>F3_QV.qual and 
		<prefix>R3_QV.qual, then the command should be:

		solid2fastq.pl <prefix>F3.csfasta <prefix>R3.csfasta \\
			<prefix>.F3_QV.qual <prefix>.R3_QV.qual
		
		It is assumed that for multi-end data, the files are given in 
		the same order as 5'->3'.
};
my $ROTATE_NUM = 100000;

getopts('', \%opts);
die($usage) if (@ARGV < 2 || 0 != (@ARGV % 2));

my @FHs = ();
my $num_ends = @ARGV/2;
my %end_counts = ();

# Open file handles
foreach my $file (@ARGV) {
	local *FH;
	open(FH, "$file") || die("Error.  Could not open $file for reading.  Terminating!\n");
	push(@FHs, *FH);
}

# Read over comments
for(my $i=0;$i<scalar(@FHs);$i++) {
	my $FH = $FHs[$i];
	my $pos = tell($FHs[$i]);
	my $line = "";
	while(defined($line = <$FH>) && $line =~ m/^#/) {
		$pos = tell($FH);
	}
	seek($FHs[$i], $pos, 0);
}

# Read in and output 
my $ctr = 0;
printf(STDERR "Outputting, currently on:\n0");
while(0 < scalar(@FHs)) {
	# Get all with min read name
	my $min_read_name = "";
	my @min_read_name_indexes = ();
	$num_ends = scalar(@FHs)/2;
	for(my $i=0;$i<$num_ends;$i++) {
		my $pos_csfasta = tell($FHs[$i]);
		my $pos_qual = tell($FHs[$i+$num_ends]);
		my $read_name = get_read_name($FHs[$i], $FHs[$i+$num_ends]);
		if(0 == length($read_name)) {
			close($FHs[$i]);
			close($FHs[$i+$num_ends]);
			# Shift down filehandles
			splice(@FHs, $i+$num_ends, 1); # shift qual first
			splice(@FHs, $i, 1); # next shift csfasta
			$num_ends = scalar(@FHs)/2;
		}
		else {
			if(0 == length($min_read_name) || 0 == cmp_read_names($read_name, $min_read_name)) {
				push(@min_read_name_indexes, $i);
				$min_read_name = $read_name;
			}
			elsif(cmp_read_names($read_name, $min_read_name) < 0) {
				@min_read_name_indexes = ();
				push(@min_read_name_indexes, $i);
			}
			# Seek back
			seek($FHs[$i], $pos_csfasta, 0);
			seek($FHs[$i+$num_ends], $pos_qual, 0);
		}
	}
	# Print all with read names
	for(my $j=0;$j<scalar(@min_read_name_indexes);$j++) {
		print_fastq($FHs[$min_read_name_indexes[$j]],
			$FHs[$min_read_name_indexes[$j]+$num_ends]);
	}
	if(0 < scalar(@min_read_name_indexes)) {
		my $key = scalar(@min_read_name_indexes);
		if(!defined($end_counts{$key})) {
			$end_counts{$key}=1;
		}
		else {
			$end_counts{$key}++;
		}
		$ctr++;
	}
	if(0 == ($ctr % $ROTATE_NUM)) {
		printf(STDERR "\r%d", $ctr);
	}
}
printf(STDERR "\r%d\n", $ctr);

printf(STDERR "Found\n%16s\t%16s\n",
	"number_of_ends",
	"number_of_reads");
my $number_of_ends = 0;
my $number_of_reads = 0;
foreach my $key (sort {$a <=> $b} (keys %end_counts)) {
	printf(STDERR "%16d\t%16d\n",
		$key,
		$end_counts{$key});
	$number_of_ends+=$key*$end_counts{$key};
	$number_of_reads+=$end_counts{$key};
}
printf(STDERR "%16s\t%16d\n",
	"total",
	$number_of_reads);
sub get_read_name {
	my $FH_csfasta = shift;;
	my $FH_qual = shift;
	my ($name_csfasta, $name_qual);

	if(!defined($name_csfasta = <$FH_csfasta>)) {
		$name_csfasta = "";
	}
	chomp($name_csfasta);
	if(!defined($name_qual = <$FH_qual>)) {
		$name_qual = "";
	}
	chomp($name_qual);
	if(0 < length($name_csfasta) && 0 < length($name_qual)) {
		if(!($name_csfasta eq $name_qual)) {
			printf(STDERR "name_csfasta=%s\nname_qual=%s\n",
				$name_csfasta,
				$name_qual);
			die("Error.  read names did not match.  Terminating!\n");
		}
	}
	return read_name_trim($name_csfasta);
}

sub print_fastq {
	my $FH_csfasta = shift;;
	my $FH_qual = shift;

	if(defined(my $name_csfasta = <$FH_csfasta>) &&
		defined(my $name_qual = <$FH_qual>) &&
		defined(my $read = <$FH_csfasta>) &&
		defined(my $qual = <$FH_qual>)) {
		chomp($name_csfasta);
		chomp($name_qual);
		chomp($read);
		chomp($qual);
		if(0 != cmp_read_names($name_csfasta, $name_qual)) {
			printf(STDERR "name_csfasta=%s\nname_qual=%s\n",
				$name_csfasta,
				$name_qual);
			die("Error.  read names did not match.  Terminating!\n");
		}
		$name_csfasta = read_name_trim($name_csfasta);
		$name_qual = read_name_trim($name_qual);
		# Convert Solid qualities
		my @quals = split(/\s+/, $qual);
		$qual = "";
		for(my $i=0;$i<scalar(@quals);$i++) {
			# Sanger encoding
			$qual .= chr(($quals[$i]<=93? $quals[$i]: 93) + 33);
		}
		# Print to stdout
		printf(STDOUT "@%s\n%s\n+\n%s\n",
			$name_csfasta,
			$read,
			$qual);
	}
	else {
		die("Error.  Could not read in.  Terminating!\n");
	}
}

sub cmp_read_names {
	my $a = shift;
	my $b = shift;

	$a = read_name_trim($a);
	$b = read_name_trim($b);

	# characters, the numbers, then characters, then numbers, ...
	# recursion is for sissies
	while(0 != length($a) &&
		0 != length($b)) {
		my $a_char = "";
		my $b_char = "";
		my $a_num = 0;
		my $b_num = 0;
		if($a =~ m/^(\d+)(.*?)$/) {
			$a_num = $1;
			$a = $2;
		}
		elsif($a =~ m/^(\D+)(.*?)$/) {
			$a_char = $1;
			$a = $2;
		}
		if($b =~ m/^(\d+)(.*?)$/) {
			$b_num = $1;
			$b = $2;
		}
		elsif($b =~ m/^(\D+)(.*?)$/) {
			$b_char = $1;
			$b = $2;
		}
		# Compare numbers then letters
		if(!($a_char eq $b_char)) {
			return ($a_char cmp $b_char);
		}
		elsif($a_num != $b_num) {
			return ($a_num <=> $b_num);
		}
	}

	return (length($a) <=> length($b));
}

sub read_name_trim {
	my $a = shift;
	# Trim last _R3 or _F3 or _whatever
	if($a =~ m/^>(.+)_[^_]*$/) {
		$a = $1;
	}
	return $a;
}
