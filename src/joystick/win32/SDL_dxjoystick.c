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
#include "SDL_dxjoystick_c.h"

/* an ISO hack for VisualC++ */
#ifdef _MSC_VER
#define   snprintf	_snprintf
#endif

#define INPUT_QSIZE	32      /* Buffer up to 32 input messages */
#define MAX_JOYSTICKS	8
#define AXIS_MIN	-32768  /* minimum value for axis coordinate */
#define AXIS_MAX	32767   /* maximum value for axis coordinate */
#define JOY_AXIS_THRESHOLD	(((AXIS_MAX)-(AXIS_MIN))/100)   /* 1% motion */

/* external variables referenced. */
extern HWND SDL_HelperWindow;


/* local variables */
static LPDIRECTINPUT dinput = NULL;
extern HRESULT(WINAPI * DInputCreate) (HINSTANCE hinst, DWORD dwVersion,
                                       LPDIRECTINPUT * ppDI,
                                       LPUNKNOWN punkOuter);
static DIDEVICEINSTANCE SYS_Joystick[MAX_JOYSTICKS];    /* array to hold joystick ID values */
static int SYS_NumJoysticks;
static HINSTANCE DInputDLL = NULL;


/* local prototypes */
static void SetDIerror(const char *function, HRESULT code);
static BOOL CALLBACK EnumJoysticksCallback(const DIDEVICEINSTANCE *
                                           pdidInstance, VOID * pContext);
static BOOL CALLBACK EnumDevObjectsCallback(LPCDIDEVICEOBJECTINSTANCE dev,
                                            LPVOID pvRef);
static Uint8 TranslatePOV(DWORD value);
static int SDL_PrivateJoystickAxis_Int(SDL_Joystick * joystick, Uint8 axis,
                                       Sint16 value);
static int SDL_PrivateJoystickHat_Int(SDL_Joystick * joystick, Uint8 hat,
                                      Uint8 value);
static int SDL_PrivateJoystickButton_Int(SDL_Joystick * joystick,
                                         Uint8 button, Uint8 state);


/* Convert a DirectInput return code to a text message */
static void
SetDIerror(const char *function, HRESULT code)
{
    SDL_SetError("%s() [%s]: %s", function,
                 DXGetErrorString8A(code), DXGetErrorDescription8A(code));
}


/* Function to scan the system for joysticks.
 * This function should set SDL_numjoysticks to the number of available
 * joysticks.  Joystick 0 should be the system default joystick.
 * It should return 0, or -1 on an unrecoverable fatal error.
 */
int
SDL_SYS_JoystickInit(void)
{
    HRESULT result;
    HINSTANCE instance;

    SYS_NumJoysticks = 0;

    result = CoInitialize(NULL);
    if (FAILED(result)) {
        SetDIerror("CoInitialize", result);
        return (-1);
    }

    result = CoCreateInstance(&CLSID_DirectInput, NULL, CLSCTX_INPROC_SERVER,
                              &IID_IDirectInput, &dinput);

    if (FAILED(result)) {
        SetDIerror("CoCreateInstance", result);
        return (-1);
    }

    /* Because we used CoCreateInstance, we need to Initialize it, first. */
    instance = GetModuleHandle(NULL);
    if (instance == NULL) {
        SDL_SetError("GetModuleHandle() failed with error code %d.",
                     GetLastError());
        return (-1);
    }
    result = IDirectInput_Initialize(dinput, instance, DIRECTINPUT_VERSION);

    if (FAILED(result)) {
        SetDIerror("IDirectInput::Initialize", result);
        return (-1);
    }

    /* Look for joysticks, wheels, head trackers, gamepads, etc.. */
    result = IDirectInput_EnumDevices(dinput,
                                      DIDEVTYPE_JOYSTICK,
                                      EnumJoysticksCallback,
                                      NULL, DIEDFL_ATTACHEDONLY);

    return SYS_NumJoysticks;
}

static BOOL CALLBACK
EnumJoysticksCallback(const DIDEVICEINSTANCE * pdidInstance, VOID * pContext)
{
    SDL_memcpy(&SYS_Joystick[SYS_NumJoysticks], pdidInstance,
               sizeof(DIDEVICEINSTANCE));
    SYS_NumJoysticks++;

    if (SYS_NumJoysticks >= MAX_JOYSTICKS)
        return DIENUM_STOP;

    return DIENUM_CONTINUE;
}

/* Function to get the device-dependent name of a joystick */
const char *
SDL_SYS_JoystickName(int index)
{
        /***-> test for invalid index ? */
    return (SYS_Joystick[index].tszProductName);
}

/* Function to open a joystick for use.
   The joystick to open is specified by the index field of the joystick.
   This should fill the nbuttons and naxes fields of the joystick structure.
   It returns 0, or -1 if there is an error.
 */
int
SDL_SYS_JoystickOpen(SDL_Joystick * joystick)
{
    HRESULT result;
    LPDIRECTINPUTDEVICE device;
    DIPROPDWORD dipdw;

    SDL_memset(&dipdw, 0, sizeof(DIPROPDWORD));
    dipdw.diph.dwSize = sizeof(DIPROPDWORD);
    dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);


    /* allocate memory for system specific hardware data */
    joystick->hwdata =
        (struct joystick_hwdata *) SDL_malloc(sizeof(struct joystick_hwdata));
    if (joystick->hwdata == NULL) {
        SDL_OutOfMemory();
        return (-1);
    }
    SDL_memset(joystick->hwdata, 0, sizeof(struct joystick_hwdata));
    joystick->hwdata->buffered = 1;
    joystick->hwdata->Capabilities.dwSize = sizeof(DIDEVCAPS);

    result =
        IDirectInput_CreateDevice(dinput,
                                  &SYS_Joystick[joystick->index].
                                  guidInstance, &device, NULL);
    if (FAILED(result)) {
        SetDIerror("IDirectInput::CreateDevice", result);
        return (-1);
    }

    /* Now get the IDirectInputDevice2 interface, instead. */
    result = IDirectInputDevice_QueryInterface(device,
                                               &IID_IDirectInputDevice2,
                                               (LPVOID *) & joystick->
                                               hwdata->InputDevice);
    /* We are done with this object.  Use the stored one from now on. */
    IDirectInputDevice_Release(device);

    if (FAILED(result)) {
        SetDIerror("IDirectInputDevice::QueryInterface", result);
        return (-1);
    }

    /* Aquire shared access. Exclusive access is required for forces,
     * though. */
    result =
        IDirectInputDevice2_SetCooperativeLevel(joystick->hwdata->
                                                InputDevice, SDL_HelperWindow,
                                                DISCL_EXCLUSIVE |
                                                DISCL_BACKGROUND);
    if (FAILED(result)) {
        SetDIerror("IDirectInputDevice2::SetCooperativeLevel", result);
        return (-1);
    }

    /* Use the extended data structure: DIJOYSTATE2. */
    result =
        IDirectInputDevice2_SetDataFormat(joystick->hwdata->InputDevice,
                                          &c_dfDIJoystick2);
    if (FAILED(result)) {
        SetDIerror("IDirectInputDevice2::SetDataFormat", result);
        return (-1);
    }

    /* Get device capabilities */
    result =
        IDirectInputDevice2_GetCapabilities(joystick->hwdata->InputDevice,
                                            &joystick->hwdata->Capabilities);

    if (FAILED(result)) {
        SetDIerror("IDirectInputDevice2::GetCapabilities", result);
        return (-1);
    }

    /* Force capable? */
    if (joystick->hwdata->Capabilities.dwFlags & DIDC_FORCEFEEDBACK) {

        result = IDirectInputDevice2_Acquire(joystick->hwdata->InputDevice);

        if (FAILED(result)) {
            SetDIerror("IDirectInputDevice2::Acquire", result);
            return (-1);
        }

        /* reset all accuators. */
        result =
            IDirectInputDevice2_SendForceFeedbackCommand(joystick->hwdata->
                                                         InputDevice,
                                                         DISFFC_RESET);

        if (FAILED(result)) {
            SetDIerror("IDirectInputDevice2::SendForceFeedbackCommand",
                       result);
            return (-1);
        }

        result = IDirectInputDevice2_Unacquire(joystick->hwdata->InputDevice);

        if (FAILED(result)) {
            SetDIerror("IDirectInputDevice2::Unacquire", result);
            return (-1);
        }

        /* Turn on auto-centering for a ForceFeedback device (until told
         * otherwise). */
        dipdw.diph.dwObj = 0;
        dipdw.diph.dwHow = DIPH_DEVICE;
        dipdw.dwData = DIPROPAUTOCENTER_ON;

        result =
            IDirectInputDevice2_SetProperty(joystick->hwdata->InputDevice,
                                            DIPROP_AUTOCENTER, &dipdw.diph);

        if (FAILED(result)) {
            SetDIerror("IDirectInputDevice2::SetProperty", result);
            return (-1);
        }
    }

    /* What buttons and axes does it have? */
    IDirectInputDevice2_EnumObjects(joystick->hwdata->InputDevice,
                                    EnumDevObjectsCallback, joystick,
                                    DIDFT_BUTTON | DIDFT_AXIS | DIDFT_POV);

    dipdw.diph.dwObj = 0;
    dipdw.diph.dwHow = DIPH_DEVICE;
    dipdw.dwData = INPUT_QSIZE;

    /* Set the buffer size */
    result =
        IDirectInputDevice2_SetProperty(joystick->hwdata->InputDevice,
                                        DIPROP_BUFFERSIZE, &dipdw.diph);

    if (result == DI_POLLEDDEVICE) {
        /* This device doesn't support buffering, so we're forced
         * to use less reliable polling. */
        joystick->hwdata->buffered = 0;
    } else if (FAILED(result)) {
        SetDIerror("IDirectInputDevice2::SetProperty", result);
        return (-1);
    }

    return (0);
}

static BOOL CALLBACK
EnumDevObjectsCallback(LPCDIDEVICEOBJECTINSTANCE dev, LPVOID pvRef)
{
    SDL_Joystick *joystick = (SDL_Joystick *) pvRef;
    HRESULT result;
    input_t *in = &joystick->hwdata->Inputs[joystick->hwdata->NumInputs];

    in->ofs = dev->dwOfs;

    if (dev->dwType & DIDFT_BUTTON) {
        in->type = BUTTON;
        in->num = joystick->nbuttons;
        joystick->nbuttons++;
    } else if (dev->dwType & DIDFT_POV) {
        in->type = HAT;
        in->num = joystick->nhats;
        joystick->nhats++;
    } else if (dev->dwType & DIDFT_AXIS) {
        DIPROPRANGE diprg;
        DIPROPDWORD dilong;

        in->type = AXIS;
        in->num = joystick->naxes;

        diprg.diph.dwSize = sizeof(diprg);
        diprg.diph.dwHeaderSize = sizeof(diprg.diph);
        diprg.diph.dwObj = dev->dwOfs;
        diprg.diph.dwHow = DIPH_BYOFFSET;
        diprg.lMin = AXIS_MIN;
        diprg.lMax = AXIS_MAX;

        result =
            IDirectInputDevice2_SetProperty(joystick->hwdata->InputDevice,
                                            DIPROP_RANGE, &diprg.diph);
        if (FAILED(result)) {
            return DIENUM_CONTINUE;     /* don't use this axis */
        }

        /* Set dead zone to 0. */
        dilong.diph.dwSize = sizeof(dilong);
        dilong.diph.dwHeaderSize = sizeof(dilong.diph);
        dilong.diph.dwObj = dev->dwOfs;
        dilong.diph.dwHow = DIPH_BYOFFSET;
        dilong.dwData = 0;
        result =
            IDirectInputDevice2_SetProperty(joystick->hwdata->InputDevice,
                                            DIPROP_DEADZONE, &dilong.diph);
        if (FAILED(result)) {
            return DIENUM_CONTINUE;     /* don't use this axis */
        }

        joystick->naxes++;
    } else {
        /* not supported at this time */
        return DIENUM_CONTINUE;
    }

    joystick->hwdata->NumInputs++;

    if (joystick->hwdata->NumInputs == MAX_INPUTS) {
        return DIENUM_STOP;     /* too many */
    }

    return DIENUM_CONTINUE;
}

/* Function to update the state of a joystick - called as a device poll.
 * This function shouldn't update the joystick structure directly,
 * but instead should call SDL_PrivateJoystick*() to deliver events
 * and update joystick device state.
 */
void
SDL_SYS_JoystickUpdate_Polled(SDL_Joystick * joystick)
{
    DIJOYSTATE2 state;
    HRESULT result;
    int i;

    result =
        IDirectInputDevice2_GetDeviceState(joystick->hwdata->InputDevice,
                                           sizeof(DIJOYSTATE2), &state);
    if (result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED) {
        IDirectInputDevice2_Acquire(joystick->hwdata->InputDevice);
        result =
            IDirectInputDevice2_GetDeviceState(joystick->hwdata->InputDevice,
                                               sizeof(DIJOYSTATE2), &state);
    }

    /* Set each known axis, button and POV. */
    for (i = 0; i < joystick->hwdata->NumInputs; ++i) {
        const input_t *in = &joystick->hwdata->Inputs[i];

        switch (in->type) {
        case AXIS:
            switch (in->ofs) {
            case DIJOFS_X:
                SDL_PrivateJoystickAxis_Int(joystick, in->num,
                                            (Sint16) state.lX);
                break;
            case DIJOFS_Y:
                SDL_PrivateJoystickAxis_Int(joystick, in->num,
                                            (Sint16) state.lY);
                break;
            case DIJOFS_Z:
                SDL_PrivateJoystickAxis_Int(joystick, in->num,
                                            (Sint16) state.lZ);
                break;
            case DIJOFS_RX:
                SDL_PrivateJoystickAxis_Int(joystick, in->num,
                                            (Sint16) state.lRx);
                break;
            case DIJOFS_RY:
                SDL_PrivateJoystickAxis_Int(joystick, in->num,
                                            (Sint16) state.lRy);
                break;
            case DIJOFS_RZ:
                SDL_PrivateJoystickAxis_Int(joystick, in->num,
                                            (Sint16) state.lRz);
                break;
            case DIJOFS_SLIDER(0):
                SDL_PrivateJoystickAxis_Int(joystick, in->num,
                                            (Sint16) state.rglSlider[0]);
                break;
            case DIJOFS_SLIDER(1):
                SDL_PrivateJoystickAxis_Int(joystick, in->num,
                                            (Sint16) state.rglSlider[1]);
                break;
            }

            break;

        case BUTTON:
            SDL_PrivateJoystickButton_Int(joystick, in->num,
                                          (Uint8) (state.
                                                   rgbButtons[in->ofs -
                                                              DIJOFS_BUTTON0]
                                                   ? SDL_PRESSED :
                                                   SDL_RELEASED));
            break;
        case HAT:
            {
                Uint8 pos = TranslatePOV(state.rgdwPOV[in->ofs -
                                                       DIJOFS_POV(0)]);
                SDL_PrivateJoystickHat_Int(joystick, in->num, pos);
                break;
            }
        }
    }
}

void
SDL_SYS_JoystickUpdate_Buffered(SDL_Joystick * joystick)
{
    int i;
    HRESULT result;
    DWORD numevents;
    DIDEVICEOBJECTDATA evtbuf[INPUT_QSIZE];

    numevents = INPUT_QSIZE;
    result =
        IDirectInputDevice2_GetDeviceData(joystick->hwdata->InputDevice,
                                          sizeof(DIDEVICEOBJECTDATA), evtbuf,
                                          &numevents, 0);
    if (result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED) {
        IDirectInputDevice2_Acquire(joystick->hwdata->InputDevice);
        result =
            IDirectInputDevice2_GetDeviceData(joystick->hwdata->InputDevice,
                                              sizeof(DIDEVICEOBJECTDATA),
                                              evtbuf, &numevents, 0);
    }

    /* Handle the events or punt */
    if (FAILED(result))
        return;

    for (i = 0; i < (int) numevents; ++i) {
        int j;

        for (j = 0; j < joystick->hwdata->NumInputs; ++j) {
            const input_t *in = &joystick->hwdata->Inputs[j];

            if (evtbuf[i].dwOfs != in->ofs)
                continue;

            switch (in->type) {
            case AXIS:
                SDL_PrivateJoystickAxis(joystick, in->num,
                                        (Sint16) evtbuf[i].dwData);
                break;
            case BUTTON:
                SDL_PrivateJoystickButton(joystick, in->num,
                                          (Uint8) (evtbuf[i].
                                                   dwData ? SDL_PRESSED :
                                                   SDL_RELEASED));
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


static Uint8
TranslatePOV(DWORD value)
{
    const int HAT_VALS[] = {
        SDL_HAT_UP,
        SDL_HAT_UP | SDL_HAT_RIGHT,
        SDL_HAT_RIGHT,
        SDL_HAT_DOWN | SDL_HAT_RIGHT,
        SDL_HAT_DOWN,
        SDL_HAT_DOWN | SDL_HAT_LEFT,
        SDL_HAT_LEFT,
        SDL_HAT_UP | SDL_HAT_LEFT
    };

    if (LOWORD(value) == 0xFFFF)
        return SDL_HAT_CENTERED;

    /* Round the value up: */
    value += 4500 / 2;
    value %= 36000;
    value /= 4500;

    if (value >= 8)
        return SDL_HAT_CENTERED;        /* shouldn't happen */

    return HAT_VALS[value];
}

/* SDL_PrivateJoystick* doesn't discard duplicate events, so we need to
 * do it. */
static int
SDL_PrivateJoystickAxis_Int(SDL_Joystick * joystick, Uint8 axis, Sint16 value)
{
    if (joystick->axes[axis] != value)
        return SDL_PrivateJoystickAxis(joystick, axis, value);
    return 0;
}

static int
SDL_PrivateJoystickHat_Int(SDL_Joystick * joystick, Uint8 hat, Uint8 value)
{
    if (joystick->hats[hat] != value)
        return SDL_PrivateJoystickHat(joystick, hat, value);
    return 0;
}

static int
SDL_PrivateJoystickButton_Int(SDL_Joystick * joystick, Uint8 button,
                              Uint8 state)
{
    if (joystick->buttons[button] != state)
        return SDL_PrivateJoystickButton(joystick, button, state);
    return 0;
}

void
SDL_SYS_JoystickUpdate(SDL_Joystick * joystick)
{
    HRESULT result;

    result = IDirectInputDevice2_Poll(joystick->hwdata->InputDevice);
    if (result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED) {
        IDirectInputDevice2_Acquire(joystick->hwdata->InputDevice);
        IDirectInputDevice2_Poll(joystick->hwdata->InputDevice);
    }

    if (joystick->hwdata->buffered)
        SDL_SYS_JoystickUpdate_Buffered(joystick);
    else
        SDL_SYS_JoystickUpdate_Polled(joystick);
}

/* Function to close a joystick after use */
void
SDL_SYS_JoystickClose(SDL_Joystick * joystick)
{
    IDirectInputDevice2_Unacquire(joystick->hwdata->InputDevice);
    IDirectInputDevice2_Release(joystick->hwdata->InputDevice);

    if (joystick->hwdata != NULL) {
        /* free system specific hardware data */
        SDL_free(joystick->hwdata);
    }
}

/* Function to perform any system-specific joystick related cleanup */
void
SDL_SYS_JoystickQuit(void)
{
    IDirectInput_Release(dinput);
    dinput = NULL;
}

#endif /* SDL_JOYSTICK_DINPUT */
/* vi: set ts=4 sw=4 expandtab: */
