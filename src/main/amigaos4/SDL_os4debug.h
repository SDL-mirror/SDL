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

/*
    Debugging output for AmigaOS 4.0.
*/

#ifndef SDL_OS4DEBUG_H
#define SDL_OS4DEBUG_H

#include <proto/exec.h>

#ifndef DEBUG
# define dprintf(format, args...)
# define kprintf(format, args...)
#else /* DEBUG */
# define dprintf(format, args...) IExec->DebugPrintF("[%s] " format, __PRETTY_FUNCTION__ , ## args)
//# define dprintf(format, args...)((struct ExecIFace *)((*(struct ExecBase **)4)->MainInterface))->DebugPrintF("[%s] " format, __PRETTY_FUNCTION__ , ## args)
# define kprintf(format, args...)((struct ExecIFace *)((*(struct ExecBase **)4)->MainInterface))->DebugPrintF(format, ## args)
#endif /* DEBUG */

#endif /* SDL_OS4DEBUG_H */
