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

#ifndef SDL_POWER_DISABLED
#ifdef SDL_POWER_LINUX

#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "SDL_power.h"

SDL_bool
SDL_GetPowerInfo_Linux_sys_power(SDL_PowerState * state,
                                 int *seconds, int *percent)
{
    return SDL_FALSE;           /* !!! FIXME: write me. */
#if 0
    const int fd = open("/sys/power", O_RDONLY);
    if (fd == -1) {
        return SDL_FALSE;       /* can't use this interface. */
    }
    return SDL_TRUE;
#endif
}

SDL_bool
SDL_GetPowerInfo_Linux_proc_acpi(SDL_PowerState * state,
                                 int *seconds, int *percent)
{
    return SDL_FALSE;           /* !!! FIXME: write me. */
#if 0
    const int fd = open("/proc/acpi", O_RDONLY);
    if (fd == -1) {
        return SDL_FALSE;       /* can't use this interface. */
    }
    return SDL_TRUE;
#endif
}

static SDL_bool
next_string(char **_ptr, char **_str)
{
    char *ptr = *_ptr;
    char *str = *_str;

    while (*ptr == ' ') {       /* skip any spaces... */
        ptr++;
    }

    if (*ptr == '\0') {
        return SDL_FALSE;
    }

    str = ptr;
    while ((*ptr != ' ') && (*ptr != '\0'))
        ptr++;

    if (*ptr != '\0')
        *(ptr++) = '\0';

    *_str = str;
    *_ptr = ptr;
    return SDL_TRUE;
}

static SDL_bool
int_string(char *str, int *val)
{
    char *endptr = NULL;
    *val = (int) strtol(str + 2, &endptr, 16);
    return ((*str != '\0') && (*endptr == '\0'));
}

/* http://lxr.linux.no/linux+v2.6.29/drivers/char/apm-emulation.c */
SDL_bool
SDL_GetPowerInfo_Linux_proc_apm(SDL_PowerState * state,
                                int *seconds, int *percent)
{
    SDL_bool need_details = SDL_FALSE;
    int ac_status = 0;
    int battery_status = 0;
    int battery_flag = 0;
    int battery_percent = 0;
    int battery_time = 0;
    const int fd = open("/proc/apm", O_RDONLY);
    char buf[128];
    char *ptr = &buf[0];
    char *str = NULL;
    ssize_t br;

    if (fd == -1) {
        return SDL_FALSE;       /* can't use this interface. */
    }

    br = read(fd, buf, sizeof(buf) - 1);
    close(fd);

    if (br < 0) {
        return SDL_FALSE;
    }

    buf[br] = '\0';             // null-terminate the string.
    if (!next_string(&ptr, &str)) {     /* driver version */
        return SDL_FALSE;
    }
    if (!next_string(&ptr, &str)) {     /* BIOS version */
        return SDL_FALSE;
    }
    if (!next_string(&ptr, &str)) {     /* APM flags */
        return SDL_FALSE;
    }

    if (!next_string(&ptr, &str)) {     /* AC line status */
        return SDL_FALSE;
    } else if (!int_string(str, &ac_status)) {
        return SDL_FALSE;
    }

    if (!next_string(&ptr, &str)) {     /* battery status */
        return SDL_FALSE;
    } else if (!int_string(str, &battery_status)) {
        return SDL_FALSE;
    }
    if (!next_string(&ptr, &str)) {     /* battery flag */
        return SDL_FALSE;
    } else if (!int_string(str, &battery_flag)) {
        return SDL_FALSE;
    }
    if (!next_string(&ptr, &str)) {     /* remaining battery life percent */
        return SDL_FALSE;
    }
    if (str[strlen(str) - 1] == '%') {
        str[strlen(str) - 1] = '\0';
    }
    if (!int_string(str, &battery_percent)) {
        return SDL_FALSE;
    }

    if (!next_string(&ptr, &str)) {     /* remaining battery life time */
        return SDL_FALSE;
    } else if (!int_string(str, &battery_time)) {
        return SDL_FALSE;
    }

    if (!next_string(&ptr, &str)) {     /* remaining battery life time units */
        return SDL_FALSE;
    } else if (strcmp(str, "min") == 0) {
        battery_time *= 60;
    }

    if (battery_flag == 0xFF) { /* unknown state */
        *state = SDL_POWERSTATE_UNKNOWN;
    } else if (battery_flag & (1 << 7)) {       /* no battery */
        *state = SDL_POWERSTATE_NO_BATTERY;
    } else if (battery_flag & (1 << 3)) {       /* charging */
        *state = SDL_POWERSTATE_CHARGING;
        need_details = SDL_TRUE;
    } else if (ac_status == 1) {
        *state = SDL_POWERSTATE_CHARGED;        /* on AC, not charging. */
        need_details = SDL_TRUE;
    } else {
        *state = SDL_POWERSTATE_ON_BATTERY;
        need_details = SDL_TRUE;
    }

    *percent = -1;
    *seconds = -1;
    if (need_details) {
        const int pct = battery_percent;
        const int secs = battery_time;

        if (pct >= 0) {         /* -1 == unknown */
            *percent = (pct > 100) ? 100 : pct; /* clamp between 0%, 100% */
        }
        if (secs >= 0) {        /* -1 == unknown */
            *seconds = secs;
        }
    }

    return SDL_TRUE;
}

#endif /* SDL_POWER_LINUX */
#endif /* SDL_POWER_DISABLED */

/* vi: set ts=4 sw=4 expandtab: */
