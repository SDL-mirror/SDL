/*
    SDL_main.c, placed in the public domain by Sam Lantinga  4/13/98

    The WinMain function -- calls your program's main() function 
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include <windows.h>
#include <malloc.h>		/* For _alloca() */

/* Include the SDL main definition header */
#include "SDL.h"
#include "SDL_main.h"
#ifdef main
#undef main
#endif

/* The standard output files */
#define STDOUT_FILE	"stdout.txt"
#define STDERR_FILE	"stderr.txt"

/* Parse a command line buffer into arguments */
static int ParseCommandLine(char *cmdline, char **argv)
{
	char *bufp;
	int argc;

	argc = 0;
	for ( bufp = cmdline; *bufp; ) {
		/* Skip leading whitespace */
		while ( isspace(*bufp) ) {
			++bufp;
		}
		/* Skip over argument */
		if ( *bufp == '"' ) {
			++bufp;
			if ( *bufp ) {
				if ( argv ) {
					argv[argc] = bufp;
				}
				++argc;
			}
			/* Skip over word */
			while ( *bufp && (*bufp != '"') ) {
				++bufp;
			}
		} else {
			if ( *bufp ) {
				if ( argv ) {
					argv[argc] = bufp;
				}
				++argc;
			}
			/* Skip over word */
			while ( *bufp && ! isspace(*bufp) ) {
				++bufp;
			}
		}
		if ( *bufp ) {
			if ( argv ) {
				*bufp = '\0';
			}
			++bufp;
		}
	}
	if ( argv ) {
		argv[argc] = NULL;
	}
	return(argc);
}

/* Show an error message */
static void ShowError(const char *title, const char *message)
{
/* If USE_MESSAGEBOX is defined, you need to link with user32.lib */
#ifdef USE_MESSAGEBOX
	MessageBox(NULL, message, title, MB_ICONEXCLAMATION|MB_OK);
#else
	fprintf(stderr, "%s: %s\n", title, message);
#endif
}

/* Pop up an out of memory message, returns to Windows */
static BOOL OutOfMemory(void)
{
	ShowError("Fatal Error", "Out of memory - aborting");
	return FALSE;
}

/* Remove the output files if there was no output written */
static void cleanup_output(void)
{
	FILE *file;
	int empty;

	/* Flush the output in case anything is queued */
	fclose(stdout);
	fclose(stderr);

	/* See if the files have any output in them */
	file = fopen(STDOUT_FILE, "rb");
	if ( file ) {
		empty = (fgetc(file) == EOF) ? 1 : 0;
		fclose(file);
		if ( empty ) {
			remove(STDOUT_FILE);
		}
	}
	file = fopen(STDERR_FILE, "rb");
	if ( file ) {
		empty = (fgetc(file) == EOF) ? 1 : 0;
		fclose(file);
		if ( empty ) {
			remove(STDERR_FILE);
		}
	}
}

#ifdef _MSC_VER /* The VC++ compiler needs main defined */
#define console_main main
#endif

/* This is where execution begins [console apps] */
int console_main(int argc, char *argv[])
{
	int n;
	char *bufp, *appname;

	/* Get the class name from argv[0] */
	appname = argv[0];
	if ( (bufp=strrchr(argv[0], '\\')) != NULL ) {
		appname = bufp+1;
	} else
	if ( (bufp=strrchr(argv[0], '/')) != NULL ) {
		appname = bufp+1;
	}

	if ( (bufp=strrchr(appname, '.')) == NULL )
		n = strlen(appname);
	else
		n = (bufp-appname);

	bufp = (char *)alloca(n+1);
	if ( bufp == NULL ) {
		return OutOfMemory();
	}
	strncpy(bufp, appname, n);
	bufp[n] = '\0';
	appname = bufp;

	/* Load SDL dynamic link library */
	if ( SDL_Init(SDL_INIT_NOPARACHUTE) < 0 ) {
		ShowError("WinMain() error", SDL_GetError());
		return(FALSE);
	}
	atexit(cleanup_output);
	atexit(SDL_Quit);

	/* Create and register our class, then run main code */
	if ( SDL_RegisterApp(appname, CS_BYTEALIGNCLIENT, 
	                     GetModuleHandle(NULL)) < 0 ) {
		ShowError("WinMain() error", SDL_GetError());
		exit(1);
	}
	SDL_main(argc, argv);

	/* Exit cleanly, calling atexit() functions */
	exit(0);
}

/* This is where execution begins [windowed apps] */
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int sw)
{
	HINSTANCE handle;
	char **argv;
	int argc;
	char *cmdline;
	char *bufp;
#ifndef NO_STDIO_REDIRECT
	FILE *newfp;
#endif

	/* Start up DDHELP.EXE before opening any files, so DDHELP doesn't
	   keep them open.  This is a hack.. hopefully it will be fixed 
	   someday.  DDHELP.EXE starts up the first time DDRAW.DLL is loaded.
	 */
	handle = LoadLibrary("DDRAW.DLL");
	if ( handle != NULL ) {
		FreeLibrary(handle);
	}

#ifndef NO_STDIO_REDIRECT
	/* Redirect standard input and standard output */
	newfp = freopen(STDOUT_FILE, "w", stdout);
	if ( newfp == NULL ) {	/* This happens on NT */
#if !defined(stdout)
		stdout = fopen(STDOUT_FILE, "w");
#else
		newfp = fopen(STDOUT_FILE, "w");
		if ( newfp ) {
			*stdout = *newfp;
		}
#endif
	}
	newfp = freopen(STDERR_FILE, "w", stderr);
	if ( newfp == NULL ) {	/* This happens on NT */
#if !defined(stderr)
		stderr = fopen(STDERR_FILE, "w");
#else
		newfp = fopen(STDERR_FILE, "w");
		if ( newfp ) {
			*stderr = *newfp;
		}
#endif
	}
	setvbuf(stdout, NULL, _IOLBF, BUFSIZ);	/* Line buffered */
	setbuf(stderr, NULL);			/* No buffering */
#endif /* !NO_STDIO_REDIRECT */

	/* Grab the command line (use alloca() on Windows) */
	bufp = GetCommandLine();
	cmdline = (char *)alloca(strlen(bufp)+1);
	if ( cmdline == NULL ) {
		return OutOfMemory();
	}
	strcpy(cmdline, bufp);

	/* Parse it into argv and argc */
	argc = ParseCommandLine(cmdline, NULL);
	argv = (char **)alloca((argc+1)*(sizeof *argv));
	if ( argv == NULL ) {
		return OutOfMemory();
	}
	ParseCommandLine(cmdline, argv);

	/* Run the main program (after a little SDL initialization) */
	return(console_main(argc, argv));
}
