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

/* Handle the BeApp specific portions of the application */

#include <AppKit.h>
#include <storage/Path.h>
#include <storage/Entry.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "SDL_BeApp.h"
#include "SDL_thread.h"
#include "SDL_timer.h"
#include "SDL_error.h"

/* Flag to tell whether or not the Be application is active or not */
int SDL_BeAppActive = 0;
static SDL_Thread *SDL_AppThread = NULL;

static int StartBeApp(void *unused)
{
	BApplication *App;

	App = new BApplication("application/x-SDL-executable");

	App->Run();
	delete App;
	return(0);
}

/* Initialize the Be Application, if it's not already started */
int SDL_InitBeApp(void)
{
	/* Create the BApplication that handles appserver interaction */
	if ( SDL_BeAppActive <= 0 ) {
		SDL_AppThread = SDL_CreateThread(StartBeApp, NULL);
		if ( SDL_AppThread == NULL ) {
			SDL_SetError("Couldn't create BApplication thread");
			return(-1);
		}
		
		/* Check if we started from Terminal or Tracker... */
		/* Based on code posted to BeDevTalk by Marco Nelissen */
		char *cmd = getenv("_"); 
		if(!(cmd == NULL || strlen(cmd) < 7) && 
			(!strcmp(cmd + strlen(cmd) - 7 , "Tracker"))) { 
	
			/* Change working to directory to that of executable */
			app_info info;
			if (B_OK == be_app->GetAppInfo(&info)) {
				entry_ref ref = info.ref;
				BEntry entry;
				if (B_OK == entry.SetTo(&ref)) {
					BPath path;
					if (B_OK == path.SetTo(&entry)) {
						if (B_OK == path.GetParent(&path)) {
							chdir(path.Path());
						}
					}
				}
			}	
		} /* else started from Terminal */
		
		do {
			SDL_Delay(10);
		} while ( (be_app == NULL) || be_app->IsLaunching() );

		/* Mark the application active */
		SDL_BeAppActive = 0;
	}

	/* Increment the application reference count */
	++SDL_BeAppActive;

	/* The app is running, and we're ready to go */
	return(0);
}

/* Quit the Be Application, if there's nothing left to do */
void SDL_QuitBeApp(void)
{
	/* Decrement the application reference count */
	--SDL_BeAppActive;

	/* If the reference count reached zero, clean up the app */
	if ( SDL_BeAppActive == 0 ) {
		if ( SDL_AppThread != NULL ) {
			if ( be_app != NULL ) { /* Not tested */
				be_app->PostMessage(B_QUIT_REQUESTED);
			}
			SDL_WaitThread(SDL_AppThread, NULL);
			SDL_AppThread = NULL;
		}
		/* be_app should now be NULL since be_app has quit */
	}
}
