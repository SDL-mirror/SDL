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
#include "SDL_config.h"

#ifdef SDL_JOYSTICK_DINPUT

/* DirectInput joystick driver; written by Glenn Maynard, based on Andrei de
 * A. Formiga's WINMM driver. 
 *
 * Hats and sliders are completely untested; the app I'm writing this for mostly
 * doesn't use them and I don't own any joysticks with them. 
 *
 * We don't bother to use event notification here.  It doesn't seem to work
 * with polled devices, and it's fine to call IDirectInputDevice2_GetDeviceData and
 * let it return 0 events. */

#include "SDL_error.h"
#include "SDL_events.h"
#include "SDL_joystick.h"
#include "../SDL_sysjoystick.h"
#include "../SDL_joystick_c.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define DIRECTINPUT_VERSION 0x0500
#include <dinput.h>

#define INPUT_QSIZE	32		/* Buffer up to 32 input messages */

extern HINSTANCE SDL_Instance;
extern int DX5_Load();
extern void DX5_Unload();
extern HRESULT (WINAPI *DInputCreate)(HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUT *ppDI, LPUNKNOWN punkOuter);

static LPDIRECTINPUT dinput = NULL;

#define MAX_JOYSTICKS	8
#define MAX_INPUTS	256	/* each joystick can have up to 256 inputs */
#define AXIS_MIN	-32768  /* minimum value for axis coordinate */
#define AXIS_MAX	32767   /* maximum value for axis coordinate */
#define JOY_AXIS_THRESHOLD	(((AXIS_MAX)-(AXIS_MIN))/100) /* 1% motion */

typedef enum Type { BUTTON, AXIS, HAT } Type;

/* array to hold joystick ID values */
static DIDEVICEINSTANCE SYS_Joystick[MAX_JOYSTICKS];
static int	SYS_NumJoysticks;

extern HWND SDL_Window;

typedef struct input_t
{
	/* DirectInput offset for this input type: */
	DWORD ofs;

	/* Button, axis or hat: */
	Type type;

	/* SDL input offset: */
	Uint8 num;
} input_t;

/* The private structure used to keep track of a joystick */
struct joystick_hwdata
{
	LPDIRECTINPUTDEVICE2 InputDevice;
	int buffered;

	input_t Inputs[MAX_INPUTS];
	int NumInputs;
};

/* Convert a DirectInput return code to a text message */
static void SetDIerror(char *function, int code)
{
	static char *error;
	static char  errbuf[1024];

	errbuf[0] = 0;
	switch (code) {
                case DIERR_GENERIC:
                        error = "Undefined error!";
                        break;
		case DIERR_OLDDIRECTINPUTVERSION:
			error = "Your version of DirectInput needs upgrading";
			break;
		case DIERR_INVALIDPARAM:
                        error = "Invalid parameters";
                        break;
                case DIERR_OUTOFMEMORY:
                        error = "Out of memory";
                        break;
		case DIERR_DEVICENOTREG:
			error = "Device not registered";
			break;
		case DIERR_NOINTERFACE:
			error = "Interface not supported";
			break;
		case DIERR_NOTINITIALIZED:
			error = "Device not initialized";
			break;
		default:
			sprintf(errbuf, "%s: Unknown DirectInput error: 0x%x",
								function, code);
			break;
	}
	if ( ! errbuf[0] ) {
		sprintf(errbuf, "%s: %s", function, error);
	}
	SDL_SetError("%s", errbuf);
	return;
}


BOOL CALLBACK EnumJoysticksCallback( const DIDEVICEINSTANCE* pdidInstance,
				     VOID* pContext )
{
	memcpy(&SYS_Joystick[SYS_NumJoysticks], pdidInstance, sizeof(DIDEVICEINSTANCE));
	SYS_NumJoysticks++;

	if( SYS_NumJoysticks >= MAX_JOYSTICKS )
	        return DIENUM_STOP;

	return DIENUM_CONTINUE;
}

static BOOL CALLBACK DIJoystick_EnumDevObjectsProc(LPCDIDEVICEOBJECTINSTANCE dev,
						    LPVOID pvRef)
{
	SDL_Joystick *joystick = (SDL_Joystick*)pvRef;
	HRESULT result;
	input_t *in = &joystick->hwdata->Inputs[joystick->hwdata->NumInputs];
	const int SupportedMask = DIDFT_BUTTON | DIDFT_POV | DIDFT_AXIS;
	if(!(dev->dwType & SupportedMask))
	    return DIENUM_CONTINUE; /* unsupported */

	in->ofs = dev->dwOfs;

	if(dev->dwType & DIDFT_BUTTON) {
		in->type = BUTTON;
		in->num = joystick->nbuttons;
		joystick->nbuttons++;
	} else if(dev->dwType & DIDFT_POV) {
		in->type = HAT;
		in->num = joystick->nhats;
		joystick->nhats++;
	} else { /* dev->dwType & DIDFT_AXIS */
		DIPROPRANGE diprg;
		DIPROPDWORD dilong;
		
		in->type = AXIS;
		in->num = joystick->naxes;
		
		diprg.diph.dwSize		= sizeof(diprg);
		diprg.diph.dwHeaderSize	= sizeof(diprg.diph);
		diprg.diph.dwObj		= dev->dwOfs;
		diprg.diph.dwHow		= DIPH_BYOFFSET;
		diprg.lMin			= AXIS_MIN;
		diprg.lMax			= AXIS_MAX;

		result = IDirectInputDevice2_SetProperty(joystick->hwdata->InputDevice, DIPROP_RANGE, &diprg.diph);
		if ( result != DI_OK )
			return DIENUM_CONTINUE; /* don't use this axis */
	
		/* Set dead zone to 0. */
		dilong.diph.dwSize		= sizeof(dilong);
		dilong.diph.dwHeaderSize	= sizeof(dilong.diph);
		dilong.diph.dwObj		= dev->dwOfs;
		dilong.diph.dwHow		= DIPH_BYOFFSET;
		dilong.dwData = 0;
		result = IDirectInputDevice2_SetProperty(joystick->hwdata->InputDevice, DIPROP_DEADZONE, &dilong.diph);
		if ( result != DI_OK )
			return DIENUM_CONTINUE; /* don't use this axis */

		joystick->naxes++;
	}

	joystick->hwdata->NumInputs++;

	if(joystick->hwdata->NumInputs == MAX_INPUTS)
		return DIENUM_STOP; /* too many */

	return DIENUM_CONTINUE;
}

/* Function to scan the system for joysticks.
 * This function should set SDL_numjoysticks to the number of available
 * joysticks.  Joystick 0 should be the system default joystick.
 * It should return 0, or -1 on an unrecoverable fatal error.
 */
int SDL_SYS_JoystickInit(void)
{
	HRESULT result;

	SYS_NumJoysticks = 0;

	/* Create the DirectInput object */
	if ( DX5_Load() < 0 ) {
		SDL_SetError("Couldn't load DirectInput");
		return(-1);
	}
	result = DInputCreate(SDL_Instance, DIRECTINPUT_VERSION,
							&dinput, NULL);
	if ( result != DI_OK ) {
		DX5_Unload();
		SetDIerror("DirectInputCreate", result);
		return(-1);
	}

	result = IDirectInput_EnumDevices(dinput,
			DIDEVTYPE_JOYSTICK, 
			EnumJoysticksCallback,
   			NULL,
			DIEDFL_ATTACHEDONLY );

	return SYS_NumJoysticks;
}

/* Function to get the device-dependent name of a joystick */
const char *SDL_SYS_JoystickName(int index)
{
	/***-> test for invalid index ? */
	return(SYS_Joystick[index].tszProductName);
}

/* Function to open a joystick for use.
   The joystick to open is specified by the index field of the joystick.
   This should fill the nbuttons and naxes fields of the joystick structure.
   It returns 0, or -1 if there is an error.
 */
int SDL_SYS_JoystickOpen(SDL_Joystick *joystick)
{
	HRESULT result;
	LPDIRECTINPUTDEVICE device;

	/* allocate memory for system specific hardware data */
	joystick->hwdata = (struct joystick_hwdata *) malloc(sizeof(*joystick->hwdata));
	if (joystick->hwdata == NULL)
	{
		SDL_OutOfMemory();
		return(-1);
	}
	memset(joystick->hwdata, 0, sizeof(*joystick->hwdata));
	joystick->hwdata->buffered = 1;
	
	result = IDirectInput_CreateDevice(dinput, &SYS_Joystick[joystick->index].guidInstance,
			    &device, NULL);
	if ( result != DI_OK ) {
		SetDIerror("DirectInput::CreateDevice", result);
		return(-1);
	}

	result = IDirectInputDevice_QueryInterface(device,
		   	    &IID_IDirectInputDevice2, (LPVOID *)&joystick->hwdata->InputDevice);
	IDirectInputDevice_Release(device);
	if ( result != DI_OK ) {
		SetDIerror("DirectInputDevice::QueryInterface", result);
		return(-1);
	}

	result = IDirectInputDevice2_SetCooperativeLevel(joystick->hwdata->InputDevice, SDL_Window,
			 DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
	if ( result != DI_OK ) {
		SetDIerror("DirectInputDevice::SetCooperativeLevel", result);
		return(-1);
	}

	result = IDirectInputDevice2_SetDataFormat(joystick->hwdata->InputDevice, &c_dfDIJoystick);
	if ( result != DI_OK ) {
		SetDIerror("DirectInputDevice::SetDataFormat", result);
		return(-1);
	}

	IDirectInputDevice2_EnumObjects(joystick->hwdata->InputDevice,
					DIJoystick_EnumDevObjectsProc,
					joystick,
					DIDFT_BUTTON | DIDFT_AXIS | DIDFT_POV);

	{
		DIPROPDWORD dipdw;
		memset(&dipdw, 0, sizeof(dipdw));
		dipdw.diph.dwSize = sizeof(dipdw);
		dipdw.diph.dwHeaderSize = sizeof(dipdw.diph);
		dipdw.diph.dwObj = 0;
		dipdw.diph.dwHow = DIPH_DEVICE;
		dipdw.dwData = INPUT_QSIZE;
		result = IDirectInputDevice2_SetProperty(joystick->hwdata->InputDevice,
						DIPROP_BUFFERSIZE, &dipdw.diph);

		if ( result == DI_POLLEDDEVICE )
		{
			/* This device doesn't support buffering, so we're forced
			 * to use less reliable polling. */
			joystick->hwdata->buffered = 0;
		} else if ( result != DI_OK ) {
			SetDIerror("DirectInputDevice::SetProperty", result);
			return(-1);
		}
	}

	return(0);
}

static Uint8 TranslatePOV(DWORD value)
{
	const int HAT_VALS[] = {
	    SDL_HAT_UP,
	    SDL_HAT_UP   | SDL_HAT_RIGHT,
	    SDL_HAT_RIGHT,
	    SDL_HAT_DOWN | SDL_HAT_RIGHT,
	    SDL_HAT_DOWN,
	    SDL_HAT_DOWN | SDL_HAT_LEFT,
	    SDL_HAT_LEFT,
	    SDL_HAT_UP   | SDL_HAT_LEFT
	};

	if(LOWORD(value) == 0xFFFF)
	    return SDL_HAT_CENTERED;

	/* Round the value up: */
	value += 4500 / 2;
	value %= 36000;
	value /= 4500;

	if(value >= 8)
	    return SDL_HAT_CENTERED; /* shouldn't happen */
	
	return HAT_VALS[value];
}

/* SDL_PrivateJoystick* doesn't discard duplicate events, so we need to
 * do it. */
static int SDL_PrivateJoystickAxis_Int(SDL_Joystick *joystick, Uint8 axis, Sint16 value)
{
	if(joystick->axes[axis] != value)
		return SDL_PrivateJoystickAxis(joystick, axis, value);
	return 0;
}

static int SDL_PrivateJoystickHat_Int(SDL_Joystick *joystick, Uint8 hat, Uint8 value)
{
	if(joystick->hats[hat] != value)
		return SDL_PrivateJoystickHat(joystick, hat, value);
	return 0;
}

static int SDL_PrivateJoystickButton_Int(SDL_Joystick *joystick, Uint8 button, Uint8 state)
{
	if(joystick->buttons[button] != state)
		return SDL_PrivateJoystickButton(joystick, button, state);
	return 0;
}

/* Function to update the state of a joystick - called as a device poll.
 * This function shouldn't update the joystick structure directly,
 * but instead should call SDL_PrivateJoystick*() to deliver events
 * and update joystick device state.
 */
void SDL_SYS_JoystickUpdate_Polled(SDL_Joystick *joystick)
{
	DIJOYSTATE state;
	HRESULT  result;
	int i;

	result = IDirectInputDevice2_GetDeviceState(joystick->hwdata->InputDevice, sizeof(state), &state);
	if ( result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED ) {
		IDirectInputDevice2_Acquire(joystick->hwdata->InputDevice);
		result = IDirectInputDevice2_GetDeviceState(joystick->hwdata->InputDevice, sizeof(state), &state);
	}

	/* Set each known axis, button and POV. */
	for(i = 0; i < joystick->hwdata->NumInputs; ++i)
	{
		const input_t *in = &joystick->hwdata->Inputs[i];

		switch(in->type)
		{
		case AXIS:
			switch(in->ofs)
			{
			case DIJOFS_X: SDL_PrivateJoystickAxis_Int(joystick, in->num, (Sint16)state.lX); break;
			case DIJOFS_Y: SDL_PrivateJoystickAxis_Int(joystick, in->num, (Sint16)state.lY); break;
			case DIJOFS_Z: SDL_PrivateJoystickAxis_Int(joystick, in->num, (Sint16)state.lZ); break;
			case DIJOFS_RX: SDL_PrivateJoystickAxis_Int(joystick, in->num, (Sint16)state.lRx); break;
			case DIJOFS_RY: SDL_PrivateJoystickAxis_Int(joystick, in->num, (Sint16)state.lRy); break;
			case DIJOFS_RZ: SDL_PrivateJoystickAxis_Int(joystick, in->num, (Sint16)state.lRz); break;
			case DIJOFS_SLIDER(0): SDL_PrivateJoystickAxis_Int(joystick, in->num, (Sint16)state.rglSlider[0]); break;
			case DIJOFS_SLIDER(1): SDL_PrivateJoystickAxis_Int(joystick, in->num, (Sint16)state.rglSlider[0]); break;
			}

			break;

		case BUTTON:
			SDL_PrivateJoystickButton_Int(joystick, in->num, (Uint8) (state.rgbButtons[in->ofs - DIJOFS_BUTTON0]?SDL_PRESSED:SDL_RELEASED));
			break;
		case HAT:
		    {
			Uint8 pos = TranslatePOV(state.rgdwPOV[in->ofs - DIJOFS_POV(0)]);
			SDL_PrivateJoystickHat_Int(joystick, in->num, pos);
			break;
		    }
		}
	}
}

void SDL_SYS_JoystickUpdate_Buffered(SDL_Joystick *joystick)
{
	int i;
	HRESULT  result;
	DWORD numevents;
	DIDEVICEOBJECTDATA evtbuf[INPUT_QSIZE];

	numevents = INPUT_QSIZE;
	result = IDirectInputDevice2_GetDeviceData(
			joystick->hwdata->InputDevice, sizeof(DIDEVICEOBJECTDATA),
						evtbuf, &numevents, 0);
	if ( result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED ) {
		IDirectInputDevice2_Acquire(joystick->hwdata->InputDevice);
		result = IDirectInputDevice2_GetDeviceData(
			joystick->hwdata->InputDevice, sizeof(DIDEVICEOBJECTDATA),
						evtbuf, &numevents, 0);
	}

	/* Handle the events */
	if ( result != DI_OK )
	    return;

	for(i = 0; i < (int) numevents; ++i)
	{
		int j;

		for(j = 0; j < joystick->hwdata->NumInputs; ++j)
		{
			const input_t *in = &joystick->hwdata->Inputs[j];

			if(evtbuf[i].dwOfs != in->ofs)
				continue;
		
			switch(in->type)
			{
			case AXIS:
				SDL_PrivateJoystickAxis(joystick, in->num, (Sint16)evtbuf[i].dwData);
				break;
			case BUTTON:
				SDL_PrivateJoystickButton(joystick, in->num, (Uint8) (evtbuf[i].dwData?SDL_PRESSED:SDL_RELEASED));
				break;
			case HAT:
			    {
				Uint8 pos = TranslatePOV(evtbuf[i].dwData);
				SDL_PrivateJoystickHat(joystick, in->num, pos);
			    }
			}
		}
	}
}

void SDL_SYS_JoystickUpdate(SDL_Joystick *joystick)
{
	HRESULT  result;

	result = IDirectInputDevice2_Poll(joystick->hwdata->InputDevice);
	if (result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED) {
		IDirectInputDevice2_Acquire(joystick->hwdata->InputDevice);
		IDirectInputDevice2_Poll(joystick->hwdata->InputDevice);
	}

	if(joystick->hwdata->buffered)
		SDL_SYS_JoystickUpdate_Buffered(joystick);
	else
		SDL_SYS_JoystickUpdate_Polled(joystick);
}

/* Function to close a joystick after use */
void SDL_SYS_JoystickClose(SDL_Joystick *joystick)
{
	IDirectInputDevice2_Unacquire(joystick->hwdata->InputDevice);
	IDirectInputDevice2_Release(joystick->hwdata->InputDevice);

	if (joystick->hwdata != NULL) {
		/* free system specific hardware data */
		free(joystick->hwdata);
	}
}

/* Function to perform any system-specific joystick related cleanup */
void SDL_SYS_JoystickQuit(void)
{
	IDirectInput_Release(dinput);
	dinput = NULL;
	DX5_Unload();
}

#endif /* SDL_JOYSTICK_DINPUT */
