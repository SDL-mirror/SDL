/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2014 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

#if defined(SDL_JOYSTICK_AMIGAINPUT) || defined(SDL_JOYSTICK_DISABLED)

#define OLDSDK 1

/* This is the Ami9gaOS4 implementation of the SDL joystick API */

#include "SDL_joystick.h"
#include "../SDL_sysjoystick.h"
#include "../SDL_joystick_c.h"

#include "SDL_events.h"

#include <amigainput/amigainput.h>
#include <proto/exec.h>
#include <proto/amigainput.h>

#define DEBUG
#include "../../main/amigaos4/SDL_os4debug.h"

/*
 * Some hard-coded maximums to simplify the code
 */
#define MAX_JOYSTICKS		32	/* How many joysticks does one need? ;-) */

#define MAX_AXES			8
#define MAX_BUTTONS			16
#define MAX_HATS			8

#define BUFFER_OFFSET(buffer, offset)	(((int32 *)buffer)[offset])

extern SDL_Joystick **SDL_joysticks;

struct joystick
{
	AIN_DeviceID	 id;
	const char      *name;
};

/* Per-joystick data private to driver */
struct joystick_hwdata
{
	AIN_DeviceHandle   *handle;
	APTR				context;

	uint32				axisBufferOffset[MAX_AXES];
	int32				axisData[MAX_AXES];
	TEXT                axisName[MAX_AXES][32];

	uint32				buttonBufferOffset[MAX_BUTTONS];
	int32				buttonData[MAX_BUTTONS];

	uint32				hatBufferOffset[MAX_HATS];
	int32				hatData[MAX_HATS];
};


static uint32 		   joystickCount;
static struct joystick joystickList [MAX_JOYSTICKS];
static APTR			   joystickContext;

/* A handy container to encapsulate the information we
 * need when enumerating joysticks on the system.
 */
struct enumPacket
{
	APTR             context;
	uint32          *count;
	struct joystick *joyList;
};

struct Library   *SDL_AIN_Base;
struct AIN_IFace *SDL_IAIN;

/**
 ** Some utility functions
 **/

static char *dupString(const char *string)
{
	char *newString = NULL;

	if (string)
	{
		uint32 len = SDL_strlen(string);

		if((newString = IExec->AllocVecTags(len + 1, TAG_DONE)))
			SDL_memcpy(newString, string, len + 1);
	}
	return newString;
}

static void freeString(char *string)
{
	if (string)
		IExec->FreeVec(string);
}

/*
 * Convert AmigaInput hat data to SDL hat data.
 */
static inline Uint8 map_hat_data(int hat_data)
{
	switch (hat_data)
	{
		case 1:  return SDL_HAT_UP;
		case 2:  return SDL_HAT_UP | SDL_HAT_RIGHT;
		case 3:  return SDL_HAT_RIGHT;
		case 4:  return SDL_HAT_DOWN | SDL_HAT_RIGHT;
		case 5:  return SDL_HAT_DOWN;
		case 6:  return SDL_HAT_DOWN | SDL_HAT_LEFT;
		case 7:  return SDL_HAT_LEFT;
		case 8:  return SDL_HAT_UP | SDL_HAT_LEFT;
		default: return SDL_HAT_CENTERED;
	}
}
/*
 * Callback to enumerate joysticks
 */
static BOOL enumerateJoysticks (AIN_Device *device, void *UserData)
{
	APTR             context =  ((struct enumPacket *)UserData)->context;
	uint32          *count   =  ((struct enumPacket *)UserData)->count;
	struct joystick *joy     = &((struct enumPacket *)UserData)->joyList[*count];

	BOOL result = FALSE;

	if (*count < MAX_JOYSTICKS)
	{
		dprintf( "ENUMJOY: id=%ld,type=%ld,axes=%ld,buttons=%ld\n",
		  count,
		  (int32)device->Type,
		  (int32)device->NumAxes,
		  (int32)device->NumButtons );
		if (device->Type == AINDT_JOYSTICK)
		{
			/* AmigaInput can report devices even when there's no
			 * physical stick present. We take some steps to try and
			 * ignore such bogus devices.
			 *
			 * First, check whether we have a useful number of axes and buttons
			 */
			if ((device->NumAxes > 0) && (device->NumButtons > 0))
			{
				/* Then, check whether we can actually obtain the device
				 */
#if OLDSDK
				AIN_DeviceHandle *handle = SDL_IAIN->AIN_ObtainDevice (context, device->DeviceID);
#else
				AIN_DeviceHandle *handle = SDL_IAIN->ObtainDevice (context, device->DeviceID);
#endif

				if (handle)
				{
					/* Okay. This appears to be a valid device. We'll report it to SDL.
					 */
					joy->id   = device->DeviceID;
					joy->name = dupString (device->DeviceName);

					dprintf("Found joystick #%d (AI ID=%d) '%s'\n", *count, joy->id, joy->name);

					(*count)++;

#if OLDSDK
					SDL_IAIN->AIN_ReleaseDevice (context, handle);
#else
					SDL_IAIN->ReleaseDevice (context, handle);
#endif

					result = TRUE;
				}
				else
					dprintf("Failed to obtain joystick '%s' (AI ID=%d) - ignoring.\n", device->DeviceName, device->DeviceID);
			}
			else
				dprintf("Joystick '%s' (AI ID=%d) has no axes/buttons - ignoring.\n", device->DeviceName, device->DeviceID);
		}
    }
	return result;
}

/**
 ** Initialization
 **/

static BOOL openAmigaInput (void)
{
	SDL_AIN_Base = IExec->OpenLibrary("AmigaInput.library", 51);

	if (SDL_AIN_Base)
	{
		SDL_IAIN = (struct AIN_IFace *) IExec->GetInterface(SDL_AIN_Base, "main", 1, NULL);

		if (!SDL_IAIN)
		{
			IExec->CloseLibrary(SDL_AIN_Base);
			SDL_AIN_Base = NULL;
		}
	}
	else
	{
		dprintf("Failed to open AmigaInput.library\n");
	}

	return SDL_AIN_Base != NULL;
}

static void closeAmigaInput(void)
{
	if (SDL_IAIN)
	{
		IExec->DropInterface((struct Interface *)SDL_IAIN);
		SDL_IAIN = NULL;
	}

	if (SDL_AIN_Base)
	{
		IExec->CloseLibrary(SDL_AIN_Base);
		SDL_AIN_Base = NULL;
	}
}

/* Function to scan the system for joysticks.
 * It should return 0, or -1 on an unrecoverable fatal error.
 */
int
SDL_SYS_JoystickInit(void)
{
	if (openAmigaInput())
	{
#if OLDSDK
		joystickContext = SDL_IAIN->AIN_CreateContext(1, NULL);
#else
		joystickContext = SDL_IAIN->CreateContext(1, NULL);
#endif

		if (joystickContext)
		{
			struct enumPacket packet = {
				 joystickContext,
				&joystickCount,
				&joystickList[0]
			};

#if OLDSDK
			dprintf( "ENUM RETURNED: %ld\n", (int32)SDL_IAIN->AIN_EnumDevices(joystickContext, enumerateJoysticks, &packet) );
#else
			dprintf( "ENUM RETURNED: %ld\n", (int32)SDL_IAIN->EnumDevices(joystickContext, enumerateJoysticks, &packet) );
#endif

			dprintf("Found %d joysticks\n", joystickCount);
		}
	}

	return joystickCount;
}

int SDL_SYS_NumJoysticks()
{
    return joystickCount;
}

void SDL_SYS_JoystickDetect()
{
}

SDL_bool SDL_SYS_JoystickNeedsPolling()
{
    return SDL_FALSE;
}

/* Function to get the device-dependent name of a joystick */
const char *
SDL_SYS_JoystickNameForDeviceIndex(int device_index)
{
	return joystickList[device_index].name;
}

/* Function to perform the mapping from device index to the instance id for this index */
SDL_JoystickID SDL_SYS_GetInstanceIdOfDeviceIndex(int device_index)
{
    return device_index;
}

/* Function to open a joystick for use.
   The joystick to open is specified by the index field of the joystick.
   This should fill the nbuttons and naxes fields of the joystick structure.
   It returns 0, or -1 if there is an error.
 */
int
SDL_SYS_JoystickOpen(SDL_Joystick * joystick, int device_index)
{
	AIN_DeviceID      id;
	AIN_DeviceHandle *handle;

	id     = joystickList[joystick->instance_id].id;
#if OLDSDK
	handle = SDL_IAIN->AIN_ObtainDevice(joystickContext, id);
#else
	handle = SDL_IAIN->ObtainDevice(joystickContext, id);
#endif

	dprintf("Opening joystick #%d (AI ID=%d)\n", joystick->instance_id, id);

	if (handle)
	{
		joystick->hwdata = IExec->AllocVecTags(sizeof (struct joystick_hwdata), AVT_ClearWithValue, 0, TAG_DONE );

		if (joystick->hwdata)
		{
			struct joystick_hwdata *hwdata		= joystick->hwdata;
			unsigned int			num_axes    = 0;
			unsigned int			num_buttons = 0;
			unsigned int			num_hats    = 0;
			TEXT tmpstr[32];
			uint32 tmpoffset;

			int i;
			BOOL result = TRUE;

			hwdata->handle  = handle;
			hwdata->context = joystickContext;

			joystick->name  = (char *) joystickList[joystick->instance_id].name;

			/* Query number of axes, buttons and hats the device has */
#if OLDSDK
			result = result && SDL_IAIN->AIN_Query(hwdata->context, id, AINQ_NUMAXES,    0, &num_axes, 4);
			result = result && SDL_IAIN->AIN_Query(hwdata->context, id, AINQ_NUMBUTTONS, 0, &num_buttons, 4);
			result = result && SDL_IAIN->AIN_Query(hwdata->context, id, AINQ_NUMHATS,    0, &num_hats, 4);
#else
			result = result && SDL_IAIN->Query(hwdata->context, id, AINQ_NUMAXES,    0, &num_axes, 4);
			result = result && SDL_IAIN->Query(hwdata->context, id, AINQ_NUMBUTTONS, 0, &num_buttons, 4);
			result = result && SDL_IAIN->Query(hwdata->context, id, AINQ_NUMHATS,    0, &num_hats, 4);
#endif

//			dprintf ("Found %d axes, %d buttons, %d hats\n", num_axes, num_buttons, num_hats);

			joystick->naxes    = num_axes < MAX_AXES       ? num_axes    : MAX_AXES;
			joystick->nbuttons = num_buttons < MAX_BUTTONS ? num_buttons : MAX_BUTTONS;
			joystick->nhats    = num_hats < MAX_HATS       ? num_hats    : MAX_HATS;
			
			// Ensure all axis names are null terminated
			for (i = 0; i < MAX_AXES; i++)
			  hwdata->axisName[i][0] = 0;

			/* Query offsets in ReadDevice buffer for axes' data */
			for (i = 0; i < joystick->naxes; i++)
			{
#if OLDSDK
				result = result && SDL_IAIN->AIN_Query(hwdata->context, id, AINQ_AXIS_OFFSET, i, &(hwdata->axisBufferOffset[i]), 4);
				result = result && SDL_IAIN->AIN_Query(hwdata->context, id, AINQ_AXISNAME,    i, &(hwdata->axisName[i][0]), 32 );
#else
				result = result && SDL_IAIN->Query(hwdata->context, id, AINQ_AXIS_OFFSET, i, &(hwdata->axisBufferOffset[i]), 4);
				result = result && SDL_IAIN->Query(hwdata->context, id, AINQ_AXISNAME,    i, &(hwdata->axisName[i][0]), 32 );
#endif
			}
			
			// Sort the axes so that X and Y come first
			for (i = 0; i < joystick->naxes; i++)
			{
			  if( ( strcasecmp( &hwdata->axisName[i][0], "X-Axis" ) == 0 ) &&
			      ( i != 0 ) )
			  {
			    // Back up the zero position axis data
			    tmpoffset = hwdata->axisBufferOffset[0];
			    strlcpy( tmpstr, hwdata->axisName[0], 32 );
			    
			    // Move this one to zero
			    hwdata->axisBufferOffset[0] = hwdata->axisBufferOffset[i];
			    strlcpy( hwdata->axisName[0], hwdata->axisName[i], 32 );
			    
			    // Put the old 0 here
			    hwdata->axisBufferOffset[i] = tmpoffset;
			    strlcpy( hwdata->axisName[i], tmpstr, 32 );
			    
			    continue;
			  }

			  if( ( strcasecmp( &hwdata->axisName[i][0], "Y-Axis" ) == 0 ) &&
			      ( i != 1 ) )
			  {
			    // Back up the position 1 axis data
			    tmpoffset = hwdata->axisBufferOffset[1];
			    strlcpy( tmpstr, hwdata->axisName[1], 32 );
			    
			    // Move this one to position 1
			    hwdata->axisBufferOffset[1] = hwdata->axisBufferOffset[i];
			    strlcpy( hwdata->axisName[1], hwdata->axisName[i], 32 );
			    
			    // Put the old 1 here
			    hwdata->axisBufferOffset[i] = tmpoffset;
			    strlcpy( hwdata->axisName[i], tmpstr, 32 );
			    
			    continue;
			  }
			}
			
			/* Query offsets in ReadDevice buffer for buttons' data */
			for (i = 0; i < joystick->nbuttons; i++)
			{
#if OLDSDK
				result = result && SDL_IAIN->AIN_Query(hwdata->context, id, AINQ_BUTTON_OFFSET, i, &(hwdata->buttonBufferOffset[i]), 4);
#else
				result = result && SDL_IAIN->Query(hwdata->context, id, AINQ_BUTTON_OFFSET, i, &(hwdata->buttonBufferOffset[i]), 4);
#endif
			}

			/* Query offsets in ReadDevice buffer for hats' data */
			for (i = 0; i < joystick->nhats; i++)
			{
#if OLDSDK
				result = result && SDL_IAIN->AIN_Query(hwdata->context, id, AINQ_HAT_OFFSET, i, &(hwdata->hatBufferOffset[i]), 4);
#else
				result = result && SDL_IAIN->Query(hwdata->context, id, AINQ_HAT_OFFSET, i, &(hwdata->hatBufferOffset[i]), 4);
#endif
			}

			if (result)
			{
				dprintf("Successful\n");

				return 0;
			}
		}
#if OLDSDK
		SDL_IAIN->AIN_ReleaseDevice (joystickContext, handle);
#else
		SDL_IAIN->ReleaseDevice (joystickContext, handle);
#endif
	}

	SDL_SetError("Failed to open device\n");

	dprintf("Failed\n");

	return -1;
}

/* Function to determine is this joystick is attached to the system right now */
SDL_bool SDL_SYS_JoystickAttached(SDL_Joystick *joystick)
{
    return SDL_TRUE;
}

/* Function to update the state of a joystick - called as a device poll.
 * This function shouldn't update the joystick structure directly,
 * but instead should call SDL_PrivateJoystick*() to deliver events
 * and update joystick device state.
 */
void
SDL_SYS_JoystickUpdate(SDL_Joystick * joystick)
{
	struct joystick_hwdata *hwdata = joystick->hwdata;
	void                   *buffer;

	/*
	 * Poll device for data
	 */
#if OLDSDK
	if (hwdata && SDL_IAIN->AIN_ReadDevice(hwdata->context, hwdata->handle, &buffer))
#else
	if (hwdata && SDL_IAIN->ReadDevice(hwdata->context, hwdata->handle, &buffer))
#endif
	{
		int i;

		/* Extract axis data from buffer and notify SDL of any changes
		 * in axis state
		 */
		for (i = 0; i < joystick->naxes; i++)
		{
			int axisdata = BUFFER_OFFSET(buffer, hwdata->axisBufferOffset[i]);

			/* Clamp axis data to 16-bits to work around possible AI driver bugs */
			if (axisdata > 32767)  axisdata =  32767;
			if (axisdata < -32768) axisdata = -32768;

			if(axisdata != hwdata->axisData[i]) {
				SDL_PrivateJoystickAxis(joystick, i, (Sint16)axisdata);
				hwdata->axisData[i] = axisdata;
			}
		}

		/* Extract button data from buffer and notify SDL of any changes
		 * in button state
		 *
		 * Note: SDL doesn't support analog buttons.
		 */
		for (i = 0; i < joystick->nbuttons; i++)
		{
			int buttondata = BUFFER_OFFSET(buffer, hwdata->buttonBufferOffset[i]);

			if(buttondata != hwdata->buttonData[i]) {
				SDL_PrivateJoystickButton(joystick, i, buttondata ? SDL_PRESSED : SDL_RELEASED);
				hwdata->buttonData[i] = buttondata;
			}
		}

		/* Extract hat data from buffer and notify SDL of any changes
		 * in hat state
		 */
		for (i = 0; i < joystick->nhats; i++)
		{
			int hatdata = BUFFER_OFFSET(buffer, hwdata->hatBufferOffset[i]);

			if(hatdata != hwdata->hatData[i]) {
				SDL_PrivateJoystickHat(joystick, i, map_hat_data(hatdata));
				hwdata->hatData[i] = hatdata;
			}
		}
	}
}

/* Function to close a joystick after use */
void
SDL_SYS_JoystickClose(SDL_Joystick * joystick)
{
	dprintf("Closing joystick #%d (AI ID=%d)\n", joystick->instance_id, joystickList[joystick->instance_id].id);

#if OLDSDK
	SDL_IAIN->AIN_ReleaseDevice(joystick->hwdata->context, joystick->hwdata->handle);
#else
	SDL_IAIN->ReleaseDevice(joystick->hwdata->context, joystick->hwdata->handle);
#endif
	IExec->FreeVec (joystick->hwdata);
	joystick->hwdata = NULL;
}

/* Function to perform any system-specific joystick related cleanup */
void
SDL_SYS_JoystickQuit(void)
{
	uint32 i;

	// PG
	// Close any open joysticks before quitting.
	// This stops a hang on exit for bad SDL software that doesn't
	// explicitly close all their joysticks.
	if( SDL_joysticks )
	{
		for ( i=0; SDL_joysticks[i]; ++i )
		{
			SDL_SYS_JoystickClose( SDL_joysticks[i] );

			/* Free the data associated with this joystick */
			if ( SDL_joysticks[i]->axes ) {
				SDL_free(SDL_joysticks[i]->axes);
			}
			if ( SDL_joysticks[i]->hats ) {
				SDL_free(SDL_joysticks[i]->hats);
			}
			if ( SDL_joysticks[i]->balls ) {
				SDL_free(SDL_joysticks[i]->balls);
			}
			if ( SDL_joysticks[i]->buttons ) {
				SDL_free(SDL_joysticks[i]->buttons);
			}
			SDL_free(SDL_joysticks[i]);
			SDL_joysticks[i] = NULL;
		}
	}

	for (i = 0; i < joystickCount; i++)
		freeString((char *)joystickList[i].name);

	joystickCount = 0;

	if (joystickContext) {
#if OLDSDK
		SDL_IAIN->AIN_DeleteContext(joystickContext);
#else
		SDL_IAIN->DeleteContext(joystickContext);
#endif
		joystickContext = 0;
	}

	closeAmigaInput();
}

SDL_JoystickGUID SDL_SYS_JoystickGetDeviceGUID( int device_index )
{
    SDL_JoystickGUID guid;
    /* the GUID is just the first 16 chars of the name for now */
    const char *name = SDL_SYS_JoystickNameForDeviceIndex( device_index );
    SDL_zero( guid );
    SDL_memcpy( &guid, name, SDL_min( sizeof(guid), SDL_strlen( name ) ) );
    return guid;
}


SDL_JoystickGUID SDL_SYS_JoystickGetGUID(SDL_Joystick * joystick)
{
    SDL_JoystickGUID guid;
    /* the GUID is just the first 16 chars of the name for now */
    const char *name = joystick->name;
    SDL_zero( guid );
    SDL_memcpy( &guid, name, SDL_min( sizeof(guid), SDL_strlen( name ) ) );
    return guid;
}

#endif /* SDL_JOYSTICK_AMIGAINPUT || SDL_JOYSTICK_DISABLED */

/* vi: set ts=4 sw=4 expandtab: */
