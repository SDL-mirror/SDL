/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2012 Sam Lantinga <slouken@libsdl.org>

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

#include "SDL_config.h"

#ifdef SDL_JOYSTICK_ANDROID

/* This is the system specific header for the SDL joystick API */
#include <stdio.h>              /* For the definition of NULL */

#include "SDL_error.h"
#include "SDL_events.h"
#include "SDL_joystick.h"
#include "../SDL_sysjoystick.h"
#include "../SDL_joystick_c.h"
#include "../../core/android/SDL_android.h"

//HACK!!
static SDL_Joystick **SYS_Joysticks;
static char **SYS_JoystickNames;
static int SYS_numjoysticks;

/* Function to convert Android keyCodes into SDL ones.
 * This code manipulation is done to get a sequential list of codes.
 */
int
keycode_to_SDL(int keycode)
{
    /* D-Pad key codes (API 1):
     * KEYCODE_DPAD_UP=19, KEYCODE_DPAD_DOWN 
     * KEYCODE_DPAD_LEFT, KEYCODE_DPAD_RIGHT, KEYCODE_DPAD_CENTER
     */
    if(keycode < 96)
        return keycode-19;
    /* Some gamepad buttons (API 9):
     * KEYCODE_BUTTON_A=96, KEYCODE_BUTTON_B, KEYCODE_BUTTON_C,
     * KEYCODE_BUTTON_X, KEYCODE_BUTTON_Y, KEYCODE_BUTTON_Z,
     * KEYCODE_BUTTON_L1, KEYCODE_BUTTON_L2,
     * KEYCODE_BUTTON_R1, KEYCODE_BUTTON_R2,
     * KEYCODE_BUTTON_THUMBL, KEYCODE_BUTTON_THUMBR,
     * KEYCODE_BUTTON_START, KEYCODE_BUTTON_SELECT, KEYCODE_BUTTON_MODE
     */
    else if(keycode < 188)
        return keycode-91;
    /* More gamepad buttons (API 12):
     * KEYCODE_BUTTON_1=188 to KEYCODE_BUTTON_16
     */
    else
        return keycode-168;
}

/* Function to scan the system for joysticks.
 * This function should set SYS_numjoysticks to the number of available
 * joysticks.  Joystick 0 should be the system default joystick.
 * It should return 0, or -1 on an unrecoverable fatal error.
 */
int
SDL_SYS_JoystickInit(void)
{
    int i = 0;
    if (Android_JNI_JoystickInit() < 0)
        return (-1);
    SYS_numjoysticks = Android_JNI_GetNumJoysticks();
    SYS_Joysticks = (SDL_Joystick **)SDL_malloc(SYS_numjoysticks*sizeof(SDL_Joystick *));
    if (SYS_Joysticks == NULL)
    {
        SDL_OutOfMemory();
        return (-1);
    }
    SYS_JoystickNames = (char **)SDL_malloc(SYS_numjoysticks*sizeof(char *));
    if (SYS_JoystickNames == NULL)
    {
        SDL_free(SYS_Joysticks);
        SYS_Joysticks = NULL;
        SDL_OutOfMemory();
        return (-1);
    }
    SDL_memset(SYS_JoystickNames, 0, (SYS_numjoysticks*sizeof(char *)));
    SDL_memset(SYS_Joysticks, 0, (SYS_numjoysticks*sizeof(SDL_Joystick *)));

    for (i = 0; i < SYS_numjoysticks; i++)
    {
        SYS_JoystickNames[i] = Android_JNI_GetJoystickName(i);
    }

    return (SYS_numjoysticks);
}

int SDL_SYS_NumJoysticks()
{
    return SYS_numjoysticks;
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
    return SYS_JoystickNames[device_index];
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
    if( device_index < SYS_numjoysticks )
    {
        // TODO: How to get the rest of the info??
        // 36 is the maximum number of handled buttons
        joystick->nbuttons = 36;
        joystick->nhats = 0;
        joystick->nballs = 0;
        joystick->naxes = Android_JNI_GetJoystickNumOfAxes(device_index);
    }
    else
    {
        return -1;
    }

    // Extremely hacky
    SYS_Joysticks[device_index] = joystick;

    return 0;
}


/* Function to update the state of a joystick - called as a device poll.
 * This function shouldn't update the joystick structure directly,
 * but instead should call SDL_PrivateJoystick*() to deliver events
 * and update joystick device state.
 */
void
SDL_SYS_JoystickUpdate(SDL_Joystick * joystick)
{
}

/* Function to determine is this joystick is attached to the system right now */
SDL_bool SDL_SYS_JoystickAttached(SDL_Joystick *joystick)
{
    return SDL_TRUE;
}

/* Function to close a joystick after use */
void
SDL_SYS_JoystickClose(SDL_Joystick * joystick)
{
}

/* Function to perform any system-specific joystick related cleanup */
void
SDL_SYS_JoystickQuit(void)
{
    SDL_free(SYS_JoystickNames);
    SDL_free(SYS_Joysticks);
    SYS_JoystickNames = NULL;
    SYS_Joysticks = NULL;
    Android_JNI_JoystickQuit();
}

SDL_JoystickGUID SDL_SYS_JoystickGetDeviceGUID( int device_index )
{
    SDL_JoystickGUID guid;
    // the GUID is just the first 16 chars of the name for now
    const char *name = SDL_SYS_JoystickNameForDeviceIndex( device_index );
    SDL_zero( guid );
    SDL_memcpy( &guid, name, SDL_min( sizeof(guid), SDL_strlen( name ) ) );
    return guid;
}

SDL_JoystickGUID SDL_SYS_JoystickGetGUID(SDL_Joystick * joystick)
{
    SDL_JoystickGUID guid;
    // the GUID is just the first 16 chars of the name for now
    const char *name = joystick->name;
    SDL_zero( guid );
    SDL_memcpy( &guid, name, SDL_min( sizeof(guid), SDL_strlen( name ) ) );
    return guid;
}

int
Android_OnPadDown(int padId, int keycode)
{
    SDL_PrivateJoystickButton(SYS_Joysticks[padId], keycode_to_SDL(keycode), SDL_PRESSED);

    return 0;
}

int
Android_OnPadUp(int padId, int keycode)
{
    SDL_PrivateJoystickButton(SYS_Joysticks[padId], keycode_to_SDL(keycode), SDL_RELEASED);

    return 0;
}

int
Android_OnJoy(int joyId, int axisnum, float value)
{
    // Android gives joy info normalized as [-1.0, 1.0] or [0.0, 1.0]
    // TODO: Are the reported values right?
    SDL_PrivateJoystickAxis(SYS_Joysticks[joyId], axisnum, (Sint16) (32767.*value) );

    return 0;
}

#endif /* SDL_JOYSTICK_ANDROID */

/* vi: set ts=4 sw=4 expandtab: */
