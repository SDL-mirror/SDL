#!/usr/bin/perl
#
# Program to take a set of header files and generate MWCC export definitions

while ( ($file = shift(@ARGV)) ) {
	if ( ! defined(open(FILE, $file)) ) {
		warn "Couldn't open $file: $!\n";
		next;
	}
	$file =~ s,.*/,,;
	while (<FILE>) {
		if ( / DECLSPEC.*SDLCALL ([^\s\(]+)/ ) {
			print "_$1\n";
		}
	}
	close(FILE);
}
# Special exports not public in the header files
