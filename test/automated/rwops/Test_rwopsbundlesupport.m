#ifdef __APPLE__
#import <Foundation/Foundation.h>

/* For proper OS X applications, the resources are contained inside the application bundle.
 So the strategy is to first check the application bundle for the file, then fallback to the current working directory.
 Note: One additional corner-case is if the resource is in a framework's resource bundle instead of the app.
 We might want to use bundle identifiers, e.g. org.libsdl.sdl to get the bundle for the framework,
 but we would somehow need to know what the bundle identifiers we need to search are.
 Also, note the bundle layouts are different for iPhone and Mac.
*/
FILE* Test_OpenFPFromBundleOrFallback(const char *file, const char *mode)
{
    FILE* fp = NULL;

	NSAutoreleasePool* autorelease_pool = [[NSAutoreleasePool alloc] init];


	NSFileManager* file_manager = [NSFileManager defaultManager];
	NSString* resource_path = [[NSBundle mainBundle] resourcePath];

	NSString* ns_string_file_component = [file_manager stringWithFileSystemRepresentation:file length:strlen(file)];

	NSString* full_path_with_file_to_try = [resource_path stringByAppendingPathComponent:ns_string_file_component];
	if([file_manager fileExistsAtPath:full_path_with_file_to_try])
	{
		fp = fopen([full_path_with_file_to_try fileSystemRepresentation], mode);
	}
	else
	{
		fp = fopen(file, mode);
	}

	[autorelease_pool drain];

	return fp;
}

FILE* Test_OpenFPFromTemporaryDir(const char *file, const char *mode)
{
    FILE* fp = NULL;
	
	NSAutoreleasePool* autorelease_pool = [[NSAutoreleasePool alloc] init];
	
	
	NSFileManager* file_manager = [NSFileManager defaultManager];
	
	NSString* ns_string_file_component = [file_manager stringWithFileSystemRepresentation:file length:strlen(file)];
	
	NSString* full_path_with_file_to_try = [NSTemporaryDirectory() stringByAppendingPathComponent:ns_string_file_component];
	fp = fopen([full_path_with_file_to_try fileSystemRepresentation], mode);
	
	[autorelease_pool drain];
	
	return fp;
}
#endif
