/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2004 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif

/* General fatal signal handling code for SDL */

#ifdef NO_SIGNAL_H

/* No signals on this platform, nothing to do.. */

void SDL_InstallParachute(void)
{
	return;
}

void SDL_UninstallParachute(void)
{
	return;
}

#else

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>

#include "SDL.h"
#include "SDL_fatal.h"

#ifdef __CYGWIN__
#define DISABLE_STDIO
#endif

/* This installs some signal handlers for the more common fatal signals,
   so that if the programmer is lazy, the app doesn't die so horribly if
   the program crashes.
*/

static void print_msg(const char *text)
{
#ifndef DISABLE_STDIO
	fprintf(stderr, "%s", text);
#endif
}

static void SDL_Parachute(int sig)
{
	signal(sig, SIG_DFL);
	print_msg("Fatal signal: ");
	switch (sig) {
		case SIGSEGV:
			print_msg("Segmentation Fault");
			break;
#ifdef SIGBUS
#if SIGBUS != SIGSEGV
		case SIGBUS:
			print_msg("Bus Error");
			break;
#endif
#endif /* SIGBUS */
#ifdef SIGFPE
		case SIGFPE:
			print_msg("Floating Point Exception");
			break;
#endif /* SIGFPE */
#ifdef SIGQUIT
		case SIGQUIT:
			print_msg("Keyboard Quit");
			break;
#endif /* SIGQUIT */
#ifdef SIGPIPE
		case SIGPIPE:
			print_msg("Broken Pipe");
			break;
#endif /* SIGPIPE */
		default:
#ifndef DISABLE_STDIO
			fprintf(stderr, "# %d", sig);
#endif
			break;
	}
	print_msg(" (SDL Parachute Deployed)\n");
	SDL_Quit();
	exit(-sig);
}

static int SDL_fatal_signals[] = {
	SIGSEGV,
#ifdef SIGBUS
	SIGBUS,
#endif
#ifdef SIGFPE
	SIGFPE,
#endif
#ifdef SIGQUIT
	SIGQUIT,
#endif
	0
};

void SDL_InstallParachute(void)
{
	/* Set a handler for any fatal signal not already handled */
	int i;
#ifdef HAVE_SIGACTION
	struct sigaction action;

	for ( i=0; SDL_fatal_signals[i]; ++i ) {
		sigaction(SDL_fatal_signals[i], NULL, &action);
		if ( action.sa_handler == SIG_DFL ) {
			action.sa_handler = SDL_Parachute;
			sigaction(SDL_fatal_signals[i], &action, NULL);
		}
	}
#ifdef SIGALRM
	/* Set SIGALRM to be ignored -- necessary on Solaris */
	sigaction(SIGALRM, NULL, &action);
	if ( action.sa_handler == SIG_DFL ) {
		action.sa_handler = SIG_IGN;
		sigaction(SIGALRM, &action, NULL);
	}
#endif
#else
	void (*ohandler)(int);

	for ( i=0; SDL_fatal_signals[i]; ++i ) {
		ohandler = signal(SDL_fatal_signals[i], SDL_Parachute);
		if ( ohandler != SIG_DFL ) {
			signal(SDL_fatal_signals[i], ohandler);
		}
	}
#endif /* HAVE_SIGACTION */
	return;
}

void SDL_UninstallParachute(void)
{
	/* Remove a handler for any fatal signal handled */
	int i;
#ifdef HAVE_SIGACTION
	struct sigaction action;

	for ( i=0; SDL_fatal_signals[i]; ++i ) {
		sigaction(SDL_fatal_signals[i], NULL, &action);
		if ( action.sa_handler == SDL_Parachute ) {
			action.sa_handler = SIG_DFL;
			sigaction(SDL_fatal_signals[i], &action, NULL);
		}
	}
#else
	void (*ohandler)(int);

	for ( i=0; SDL_fatal_signals[i]; ++i ) {
		ohandler = signal(SDL_fatal_signals[i], SIG_DFL);
		if ( ohandler != SDL_Parachute ) {
			signal(SDL_fatal_signals[i], ohandler);
		}
	}
#endif /* HAVE_SIGACTION */
}

#endif /* NO_SIGNAL_H */
