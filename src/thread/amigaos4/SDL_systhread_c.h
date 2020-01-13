/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2019 Sam Lantinga <slouken@libsdl.org>

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

#ifndef SDL_SYSTHREAD_C_H
#define SDL_SYSTHREAD_C_H

#include "../../timer/amigaos4/SDL_os4timer_c.h"

typedef struct Task* SYS_ThreadHandle;

void OS4_InitThreadSubSystem(void);
void OS4_QuitThreadSubSystem(void);

OS4_TimerInstance* OS4_ThreadGetTimer(void);

#endif

