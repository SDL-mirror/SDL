#!/usr/bin/perl
#
# Program to take a set of header files and generate DLL export definitions

# Special exports to ignore for this platform
$exclude{"SDL_putenv"} = 1;
$exclude{"SDL_getenv"} = 1;
$exclude{"SDL_CreateThread_Core"} = 1;

while ( ($file = shift(@ARGV)) ) {
	if ( ! defined(open(FILE, $file)) ) {
		warn "Couldn't open $file: $!\n";
		next;
	}
	$file =~ s,.*/,,;
	while (<FILE>) {
		if ( / DECLSPEC.* SDLCALL ([^\s\(]+)/ ) {
			if ( not $exclude{$1} ) {
				print "\t_$1\n";
			}
		}
	}
	close(FILE);
}

# Special exports to include for this platform
print "\t.objc_class_name_SDL_QuartzWindow\n";
print "\t.objc_class_name_SDL_QuartzWindowDelegate\n";
