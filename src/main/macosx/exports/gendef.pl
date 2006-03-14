#!/usr/bin/perl
#
# Program to take a set of header files and generate DLL export definitions

# Special exports to ignore for this platform

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
print "\t_SDL_strrev\n";
print "\t_SDL_strupr\n";
print "\t_SDL_strlwr\n";
print "\t_SDL_ltoa\n";
print "\t_SDL_ultoa\n";
print "\t_SDL_lltoa\n";
print "\t_SDL_ulltoa\n";
print "\t_SDL_iconv\n";
print "\t_SDL_iconv_string\n";
print "\t.objc_class_name_SDL_QuartzWindow\n";
print "\t.objc_class_name_SDL_QuartzWindowDelegate\n";
