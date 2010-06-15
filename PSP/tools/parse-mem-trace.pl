#!/usr/bin/perl
#
# Aquaria PSP port
# Copyright (C) 2010 Andrew Church <achurch@achurch.org>
#
# tools/parse-mem-trace: Quick-and-dirty script to parse a memory allocation
# trace log (as output by TRACE_ALLOCS in src/{malloc,memory}.c) and print
# the amount of memory owned by each caller at the end of the log.

%owner = ();	# Owner of each memory block
%size = ();	# Allocated size of each memory block
%full = ();	# Actual space used for each memory block
%owned = ();	# Allocated bytes owned by each caller
%used = ();	# Actual space used by each caller

while (<>) {
	if (/\[(.*?)\] [mc]?alloc\((\d+)(?:,\d+)?\) -\> (0x[0-9A-F]+)/
	 || /\[(.*?)\] realloc\(0x0,(\d+)\) -\> (0x[0-9A-F]+)/) {
		my ($caller, $size, $ptr) = ($1, $2, $3);
		my $used = int(($size+48+63)/64) * 64;
		if (m@: src/malloc@) {
			$caller = "$caller [malloc]";
			$used = int(($size+8+7)/8) * 8;
		}
		$owner{$ptr} = $caller;
		$size{$ptr} = $size;
		$full{$ptr} = $used;
		$owned{$caller} += $size;
		$used{$caller} += $used;
	} elsif (/\[(.*?)\] realloc\((0x[0-9A-F]+),(\d+),\d+\) -\> (0x[0-9A-F]+)/) {
		my ($caller, $old, $size, $ptr) = ($1, $2, $3, $4);
		if (!$owner{$old}) {
			print STDERR "WARNING: $old has no owner, skipping\n";
			next;
		}
		$owned{$owner{$old}} -= $size{$old};
		$used{$owner{$old}} -= $full{$old};
		delete $owner{$old};
		delete $size{$old};
		delete $full{$old};
		my $used = int(($size+48+63)/64) * 64;
		if (m@: src/malloc@) {
			$caller = "$caller [malloc]";
			$used = int(($size+8+7)/8) * 8;
		}
		$owner{$ptr} = $caller;
		$size{$ptr} = $size;
		$full{$ptr} = $used;
		$owned{$caller} += $size;
		$used{$caller} += $used;
	} elsif (/\[(.*?)\] free\((0x[0-9A-F]+)\)/ || /\[(.*?)\] realloc\((0x[0-9A-F]+),\d+\) -\> free/) {
		my ($caller, $ptr) = ($1, $2);
		if (!$owner{$ptr}) {
			print STDERR "WARNING: $ptr has no owner, skipping\n";
			next;
		}
		$owned{$owner{$ptr}} -= $size{$ptr};
		$used{$owner{$ptr}} -= $full{$ptr};
		delete $owner{$ptr};
		delete $size{$ptr};
		delete $full{$ptr};
	} elsif (/\] ([mc]?alloc|realloc|free)/) {
		chomp;
		die "Unable to parse: $_";
	}
}

printf "Alloc'ed  SizeUsed  Caller\n";
printf "--------  --------  ------\n";
foreach (sort {$owned{$b} <=> $owned{$a}} keys %owned) {
	printf "%8d  %8d  %s\n",$owned{$_},$used{$_},$_;
}
