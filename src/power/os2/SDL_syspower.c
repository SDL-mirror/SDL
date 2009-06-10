/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2009 Sam Lantinga

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

/* !!! FIXME:
 * Please note that this code has not been tested (or even compiled!). It
 *  should, in theory, run on any version of OS/2, and work with any system
 *  that has APM.SYS loaded. I don't know if ACPI.SYS works.
 */

#ifndef SDL_POWER_DISABLED
#ifdef SDL_POWER_OS2

#define INCL_DOSFILEMGR
#define INCL_DOSDEVICES
#define INCL_DOSDEVIOCTL
#define INCL_DOSERRORS
#include <os2.h>

#include "SDL_power.h"

typedef struct
{
    USHORT len;
    USHORT flags;
    UCHAR ac_status;
    UCHAR battery_status;
    UCHAR battery_life;
    UCHAR battery_time_form;
    USHORT battery_time;
    UCHAR battery_flags;
} PowerStatus;
extern int CompilerAssertPowerStatus[(sizeof(PowerStatus) == 10) ? 1 : -1];


SDL_bool
SDL_GetPowerInfo_OS2(SDL_PowerState * state, int *seconds, int *percent)
{
    PowerStatus status;
    HFILE hfile = 0;
    ULONG action = 0;
    APIRET rc = 0;

    *state = SDL_POWERSTATE_UNKNOWN;
    *percent = -1;
    *seconds = -1;

    /* open the power management device */
    rc = DosOpen("APM$", &hfile, &action, 0, FILE_NORMAL, FILE_OPEN,
                 OPEN_ACCESS_READONLY | OPEN_SHARE_DENYNONE, 0);

    if (rc == NO_ERROR) {
        USHORT iorc = 0;
        ULONG iorclen = sizeof(iorc);
        ULONG statuslen = sizeof(status);

        SDL_memset(&status, '\0', sizeof(status));
        status.len = sizeof(status);

        rc = DosDevIOCtl(hfile, IOCTL_POWER, POWER_GETPOWERSTATUS, &status,
                         statuslen, &statuslen, &iorc, iorclen, &iorclen);
        DosClose(hfile);

        /* (status.flags & 0x1) == power subsystem enabled. */
        if ((rc == NO_ERROR) && (status.flags & 0x1)) {
            if (statuslen == 7) {       /* older OS/2 APM driver? Less fields. */
                status.battery_time_form = 0xFF;
                status.battery_time = 0;
                if (status.battery_status == 0xFF) {
                    status.battery_flags = 0xFF;
                } else {
                    status.battery_flags = (1 << status.battery_status);
                }
            }

            if (status.battery_flags == 0xFF) { /* unknown state */
                *state = SDL_POWERSTATE_UNKNOWN;
            } else if (status.battery_flags & (1 << 7)) {       /* no battery */
                *state = SDL_POWERSTATE_NO_BATTERY;
            } else if (status.battery_flags & (1 << 3)) {       /* charging */
                *state = SDL_POWERSTATE_CHARGING;
                need_details = SDL_TRUE;
            } else if (status.ac_status == 1) {
                *state = SDL_POWERSTATE_CHARGED;        /* on AC, not charging. */
                need_details = SDL_TRUE;
            } else {
                *state = SDL_POWERSTATE_ON_BATTERY;     /* not on AC. */
                need_details = SDL_TRUE;
            }

            if (need_details) {
                const int pct = (int) status.battery_life;
                const int secs = (int) status.battery_time;

                if (pct != 0xFF) {      /* 255 == unknown */
                    *percent = (pct > 100) ? 100 : pct;
                }

                if (status.battery_time_form == 0xFF) { /* unknown */
                    *seconds = -1;
                } else if (status.battery_time_form == 1) {     /* minutes */
                    *seconds = secs * 60;
                } else {
                    *seconds = secs;
                }
            }
        }
    }

    return SDL_TRUE;            /* always the definitive answer on OS/2. */
}

#endif /* SDL_POWER_OS2 */
#endif /* SDL_POWER_DISABLED */

/* vi: set ts=4 sw=4 expandtab: */
