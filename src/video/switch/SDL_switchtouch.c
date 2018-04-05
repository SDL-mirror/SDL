/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2017 Sam Lantinga <slouken@libsdl.org>

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

#if SDL_VIDEO_DRIVER_SWITCH

#include <switch.h>

#include "SDL_events.h"
#include "SDL_switchtouch.h"
#include "../../events/SDL_touch_c.h"

#define MAX_TOUCH 16

typedef struct SwitchTouch
{
    SDL_FingerID id;
    touchPosition position;
} SwitchTouch;

typedef struct TouchState
{
    SwitchTouch touch[MAX_TOUCH];
    u32 count;
} TouchState;

TouchState touchState;

TouchState touchState_old;

void
SWITCH_InitTouch(void)
{
    SDL_AddTouch((SDL_TouchID) 0, "Switch");
}

void
SWITCH_QuitTouch(void)
{

}

void
SWITCH_PollTouch(void)
{
    memcpy(&touchState_old, &touchState, sizeof(touchState));

    touchState.count = hidTouchCount();
    if (touchState.count >= MAX_TOUCH) {
        touchState.count = MAX_TOUCH - 1;
    }

    if (touchState.count > 0) {
        for (u32 i = 0; i < touchState.count; i++) {

            touchState.touch[i].id = i;
            hidTouchRead(&touchState.touch[i].position, i);

            // Send an initial touch
            SDL_SendTouch(0, (SDL_FingerID) i, SDL_TRUE,
                          (float) touchState.touch[i].position.px / 1280.0f,
                          (float) touchState.touch[i].position.py / 720.0f, 1);

            // Always send the motion
            SDL_SendTouchMotion(0, (SDL_FingerID) i,
                                (float) touchState.touch[i].position.px / 1280.0f,
                                (float) touchState.touch[i].position.py / 720.0f, 1);
        }
    }

    // some fingers might have been let go
    if (touchState_old.count > 0) {

        for (int i = 0; i < touchState_old.count; i++) {

            int finger_up = 1;
            if (touchState.count > 0) {
                for (int j = 0; j < touchState.count; j++) {
                    if (touchState.touch[j].id == touchState_old.touch[i].id) {
                        finger_up = 0;
                    }
                }
            }

            if (finger_up == 1) {
                // Finger released from screen
                SDL_SendTouch((SDL_TouchID) 0, (SDL_FingerID) touchState_old.touch[i].id, SDL_FALSE,
                              (float) touchState_old.touch[i].position.px / 1280.0f,
                              (float) touchState_old.touch[i].position.py / 720.0f, 1);
            }
        }
    }
}

#endif /* SDL_VIDEO_DRIVER_SWITCH */

/* vi: set ts=4 sw=4 expandtab: */
