/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2006 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/

/* Not all environments have a working getenv()/putenv() */

#ifdef TEST_MAIN
#define NEED_SDL_GETENV
#endif

#include "SDL_getenv.h"

#ifdef NEED_SDL_GETENV

#if defined(WIN32) && !defined(_WIN32_WCE)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <malloc.h>
#include <string.h>

/* Note this isn't thread-safe! */

static char *SDL_envmem = NULL;	/* Ugh, memory leak */
static DWORD SDL_envmemlen = 0;

/* Put a variable of the form "name=value" into the environment */
int SDL_putenv(const char *variable)
{
	DWORD bufferlen;
	char *value;
	const char *sep;

	sep = strchr(variable, '=');
	if ( sep == NULL ) {
		return -1;
	}
	bufferlen = strlen(variable)+1;
	if ( bufferlen > SDL_envmemlen ) {
		char *newmem = (char *)realloc(SDL_envmem, bufferlen);
		if ( newmem == NULL ) {
			return -1;
		}
		SDL_envmem = newmem;
		SDL_envmemlen = bufferlen;
	}
	strcpy(SDL_envmem, variable);
	value = SDL_envmem + (sep - variable);
	*value++ = '\0';
	if ( !SetEnvironmentVariable(SDL_envmem, *value ? value : NULL) ) {
		return -1;
	}
	return 0;
}

/* Retrieve a variable named "name" from the environment */
char *SDL_getenv(const char *name)
{
	DWORD bufferlen;

	bufferlen = GetEnvironmentVariable(name, SDL_envmem, SDL_envmemlen);
	if ( bufferlen == 0 ) {
		return NULL;
	}
	if ( bufferlen > SDL_envmemlen ) {
		char *newmem = (char *)realloc(SDL_envmem, bufferlen);
		if ( newmem == NULL ) {
			return NULL;
		}
		SDL_envmem = newmem;
		SDL_envmemlen = bufferlen;
		GetEnvironmentVariable(name, SDL_envmem, SDL_envmemlen);
	}
	return SDL_envmem;
}

#else /* roll our own */

#include <stdlib.h>
#include <string.h>

static char **SDL_env = (char **)0;

/* Put a variable of the form "name=value" into the environment */
int SDL_putenv(const char *variable)
{
	const char *name, *value;
	int added;
	int len, i;
	char **new_env;
	char *new_variable;

	/* A little error checking */
	if ( ! variable ) {
		return(-1);
	}
	name = variable;
	for ( value=variable; *value && (*value != '='); ++value ) {
		/* Keep looking for '=' */ ;
	}
	if ( *value ) {
		++value;
	} else {
		return(-1);
	}

	/* Allocate memory for the variable */
	new_variable = (char *)malloc(strlen(variable)+1);
	if ( ! new_variable ) {
		return(-1);
	}
	strcpy(new_variable, variable);

	/* Actually put it into the environment */
	added = 0;
	i = 0;
	if ( SDL_env ) {
		/* Check to see if it's already there... */
		len = (value - name);
		for ( ; SDL_env[i]; ++i ) {
			if ( strncmp(SDL_env[i], name, len) == 0 ) {
				break;
			}
		}
		/* If we found it, just replace the entry */
		if ( SDL_env[i] ) {
			free(SDL_env[i]);
			SDL_env[i] = new_variable;
			added = 1;
		}
	}

	/* Didn't find it in the environment, expand and add */
	if ( ! added ) {
		new_env = realloc(SDL_env, (i+2)*sizeof(char *));
		if ( new_env ) {
			SDL_env = new_env;
			SDL_env[i++] = new_variable;
			SDL_env[i++] = (char *)0;
			added = 1;
		} else {
			free(new_variable);
		}
	}
	return (added ? 0 : -1);
}

/* Retrieve a variable named "name" from the environment */
char *SDL_getenv(const char *name)
{
	int len, i;
	char *value;

	value = (char *)0;
	if ( SDL_env ) {
		len = strlen(name);
		for ( i=0; SDL_env[i] && !value; ++i ) {
			if ( (strncmp(SDL_env[i], name, len) == 0) &&
			     (SDL_env[i][len] == '=') ) {
				value = &SDL_env[i][len+1];
			}
		}
	}
	return value;
}

#endif /* WIN32 */

#endif /* NEED_GETENV */

#ifdef TEST_MAIN
#include <stdio.h>

int main(int argc, char *argv[])
{
	char *value;

	printf("Checking for non-existent variable... ");
	fflush(stdout);
	if ( ! getenv("EXISTS") ) {
		printf("okay\n");
	} else {
		printf("failed\n");
	}
	printf("Setting FIRST=VALUE1 in the environment... ");
	fflush(stdout);
	if ( putenv("FIRST=VALUE1") == 0 ) {
		printf("okay\n");
	} else {
		printf("failed\n");
	}
	printf("Getting FIRST from the environment... ");
	fflush(stdout);
	value = getenv("FIRST");
	if ( value && (strcmp(value, "VALUE1") == 0) ) {
		printf("okay\n");
	} else {
		printf("failed\n");
	}
	printf("Setting SECOND=VALUE2 in the environment... ");
	fflush(stdout);
	if ( putenv("SECOND=VALUE2") == 0 ) {
		printf("okay\n");
	} else {
		printf("failed\n");
	}
	printf("Getting SECOND from the environment... ");
	fflush(stdout);
	value = getenv("SECOND");
	if ( value && (strcmp(value, "VALUE2") == 0) ) {
		printf("okay\n");
	} else {
		printf("failed\n");
	}
	printf("Setting FIRST=NOVALUE in the environment... ");
	fflush(stdout);
	if ( putenv("FIRST=NOVALUE") == 0 ) {
		printf("okay\n");
	} else {
		printf("failed\n");
	}
	printf("Getting FIRST from the environment... ");
	fflush(stdout);
	value = getenv("FIRST");
	if ( value && (strcmp(value, "NOVALUE") == 0) ) {
		printf("okay\n");
	} else {
		printf("failed\n");
	}
	printf("Checking for non-existent variable... ");
	fflush(stdout);
	if ( ! getenv("EXISTS") ) {
		printf("okay\n");
	} else {
		printf("failed\n");
	}
	return(0);
}
#endif /* TEST_MAIN */

