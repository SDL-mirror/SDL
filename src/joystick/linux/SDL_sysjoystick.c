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

#ifdef SDL_JOYSTICK_LINUX

/* This is the system specific header for the SDL joystick API */

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <limits.h>             /* For the definition of PATH_MAX */
#include <linux/joystick.h>

#include "SDL_joystick.h"
#include "../SDL_sysjoystick.h"
#include "../SDL_joystick_c.h"
#include "SDL_sysjoystick_c.h"

/* Special joystick configurations */
static struct
{
    const char *name;
    int naxes;
    int nhats;
    int nballs;
} special_joysticks[] = {
    {
    "MadCatz Panther XL", 3, 2, 1},     /* We don't handle rudder (axis 8) */
    {
    "SideWinder Precision Pro", 4, 1, 0}, {
    "SideWinder 3D Pro", 4, 1, 0}, {
    "Microsoft SideWinder 3D Pro", 4, 1, 0}, {
    "Microsoft SideWinder Precision Pro", 4, 1, 0}, {
    "Microsoft SideWinder Dual Strike USB version 1.0", 2, 1, 0}, {
    "WingMan Interceptor", 3, 3, 0}, {
    "WingMan Extreme Digital 3D", 4, 1, 0}, {
    "Microsoft SideWinder Precision 2 Joystick", 4, 1, 0}, {
    "Logitech Inc. WingMan Extreme Digital 3D", 4, 1, 0}, {
    "Saitek Saitek X45", 6, 1, 0}
};

/* The maximum number of joysticks we'll detect */
#define MAX_JOYSTICKS	32

/* A list of available joysticks */
static struct
{
    char *fname;
} SDL_joylist[MAX_JOYSTICKS];


#if SDL_INPUT_LINUXEV
#define test_bit(nr, addr) \
	(((1UL << ((nr) % (sizeof(long) * 8))) & ((addr)[(nr) / (sizeof(long) * 8)])) != 0)
#define NBITS(x) ((((x)-1)/(sizeof(long) * 8))+1)

static int
EV_IsJoystick(int fd)
{
    unsigned long evbit[NBITS(EV_MAX)] = { 0 };
    unsigned long keybit[NBITS(KEY_MAX)] = { 0 };
    unsigned long absbit[NBITS(ABS_MAX)] = { 0 };

    if ((ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), evbit) < 0) ||
        (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit) < 0) ||
        (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absbit)), absbit) < 0)) {
        return (0);
    }

    if (!(test_bit(EV_KEY, evbit) && test_bit(EV_ABS, evbit) &&
          test_bit(ABS_X, absbit) && test_bit(ABS_Y, absbit))) {
        return 0;
    }
    return (1);
}

#endif /* SDL_INPUT_LINUXEV */

static int SDL_SYS_numjoysticks = 0;

/* Function to scan the system for joysticks */
int
SDL_SYS_JoystickInit(void)
{
    /* The base path of the joystick devices */
    const char *joydev_pattern[] = {
#if SDL_INPUT_LINUXEV
        "/dev/input/event%d",
#endif
        "/dev/input/js%d",
        "/dev/js%d"
    };
    int numjoysticks;
    int i, j;
    int fd;
    char path[PATH_MAX];
    dev_t dev_nums[MAX_JOYSTICKS];      /* major/minor device numbers */
    struct stat sb;
    int n, duplicate;

    numjoysticks = 0;

    /* First see if the user specified one or more joysticks to use */
    if (SDL_getenv("SDL_JOYSTICK_DEVICE") != NULL) {
        char *envcopy, *envpath, *delim;
        envcopy = SDL_strdup(SDL_getenv("SDL_JOYSTICK_DEVICE"));
        envpath = envcopy;
        while (envpath != NULL) {
            delim = SDL_strchr(envpath, ':');
            if (delim != NULL) {
                *delim++ = '\0';
            }
            if (stat(envpath, &sb) == 0) {
                fd = open(envpath, O_RDONLY, 0);
                if (fd >= 0) {
                    /* Assume the user knows what they're doing. */
                    SDL_joylist[numjoysticks].fname = SDL_strdup(envpath);
                    if (SDL_joylist[numjoysticks].fname) {
                        dev_nums[numjoysticks] = sb.st_rdev;
                        ++numjoysticks;
                    }
                    close(fd);
                }
            }
            envpath = delim;
        }
        SDL_free(envcopy);
    }

    for (i = 0; i < SDL_arraysize(joydev_pattern); ++i) {
        for (j = 0; j < MAX_JOYSTICKS; ++j) {
            SDL_snprintf(path, SDL_arraysize(path), joydev_pattern[i], j);

            /* rcg06302000 replaced access(F_OK) call with stat().
             * stat() will fail if the file doesn't exist, so it's
             * equivalent behaviour.
             */
            if (stat(path, &sb) == 0) {
                /* Check to make sure it's not already in list.
                 * This happens when we see a stick via symlink.
                 */
                duplicate = 0;
                for (n = 0; (n < numjoysticks) && !duplicate; ++n) {
                    if (sb.st_rdev == dev_nums[n]) {
                        duplicate = 1;
                    }
                }
                if (duplicate) {
                    continue;
                }

                fd = open(path, O_RDONLY, 0);
                if (fd < 0) {
                    continue;
                }
#if SDL_INPUT_LINUXEV
#ifdef DEBUG_INPUT_EVENTS
                printf("Checking %s\n", path);
#endif
                if ((i == 0) && !EV_IsJoystick(fd)) {
                    close(fd);
                    continue;
                }
#endif
                close(fd);

                /* We're fine, add this joystick */
                SDL_joylist[numjoysticks].fname = SDL_strdup(path);
                if (SDL_joylist[numjoysticks].fname) {
                    dev_nums[numjoysticks] = sb.st_rdev;
                    ++numjoysticks;
                }
            }
        }

#if SDL_INPUT_LINUXEV
        /* This is a special case...
           If the event devices are valid then the joystick devices
           will be duplicates but without extra information about their
           hats or balls. Unfortunately, the event devices can't
           currently be calibrated, so it's a win-lose situation.
           So : /dev/input/eventX = /dev/input/jsY = /dev/js
         */
        if ((i == 0) && (numjoysticks > 0))
            break;
#endif
    }

    SDL_SYS_numjoysticks = numjoysticks;
    return (numjoysticks);
}

int SDL_SYS_NumJoysticks()
{
    return SDL_SYS_numjoysticks;
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
    int fd;
    static char namebuf[128];
    const char *name;

    name = NULL;
    fd = open(SDL_joylist[device_index].fname, O_RDONLY, 0);
    if (fd >= 0) {
        if (
#if SDL_INPUT_LINUXEV
               (ioctl(fd, EVIOCGNAME(sizeof(namebuf)), namebuf) <= 0) &&
#endif
               (ioctl(fd, JSIOCGNAME(sizeof(namebuf)), namebuf) <= 0)) {
            name = SDL_joylist[device_index].fname;
        } else {
            name = namebuf;
        }
        close(fd);
    }
    return name;
}

/* Function to perform the mapping from device index to the instance id for this index */
SDL_JoystickID SDL_SYS_GetInstanceIdOfDeviceIndex(int device_index)
{
    return device_index;
}

static int
allocate_hatdata(SDL_Joystick * joystick)
{
    int i;

    joystick->hwdata->hats =
        (struct hwdata_hat *) SDL_malloc(joystick->nhats *
                                         sizeof(struct hwdata_hat));
    if (joystick->hwdata->hats == NULL) {
        return (-1);
    }
    for (i = 0; i < joystick->nhats; ++i) {
        joystick->hwdata->hats[i].axis[0] = 1;
        joystick->hwdata->hats[i].axis[1] = 1;
    }
    return (0);
}

static int
allocate_balldata(SDL_Joystick * joystick)
{
    int i;

    joystick->hwdata->balls =
        (struct hwdata_ball *) SDL_malloc(joystick->nballs *
                                          sizeof(struct hwdata_ball));
    if (joystick->hwdata->balls == NULL) {
        return (-1);
    }
    for (i = 0; i < joystick->nballs; ++i) {
        joystick->hwdata->balls[i].axis[0] = 0;
        joystick->hwdata->balls[i].axis[1] = 0;
    }
    return (0);
}

static SDL_bool
JS_ConfigJoystick(SDL_Joystick * joystick, int fd)
{
    SDL_bool handled;
    unsigned char n;
    int tmp_naxes, tmp_nhats, tmp_nballs;
    const char *name;
    char *env, env_name[128];
    int i;

    handled = SDL_FALSE;

    /* Default joystick device settings */
    if (ioctl(fd, JSIOCGAXES, &n) < 0) {
        joystick->naxes = 2;
    } else {
        joystick->naxes = n;
    }
    if (ioctl(fd, JSIOCGBUTTONS, &n) < 0) {
        joystick->nbuttons = 2;
    } else {
        joystick->nbuttons = n;
    }

    name = SDL_SYS_JoystickNameForDeviceIndex(joystick->instance_id);

    /* Generic analog joystick support */
    if (SDL_strstr(name, "Analog") == name && SDL_strstr(name, "-hat")) {
        if (SDL_sscanf(name, "Analog %d-axis %*d-button %d-hat",
                       &tmp_naxes, &tmp_nhats) == 2) {

            joystick->naxes = tmp_naxes;
            joystick->nhats = tmp_nhats;

            handled = SDL_TRUE;
        }
    }

    /* Special joystick support */
    for (i = 0; i < SDL_arraysize(special_joysticks); ++i) {
        if (SDL_strcmp(name, special_joysticks[i].name) == 0) {

            joystick->naxes = special_joysticks[i].naxes;
            joystick->nhats = special_joysticks[i].nhats;
            joystick->nballs = special_joysticks[i].nballs;

            handled = SDL_TRUE;
            break;
        }
    }

    /* User environment joystick support */
    if ((env = SDL_getenv("SDL_LINUX_JOYSTICK"))) {
        *env_name = '\0';
        if (*env == '\'' && SDL_sscanf(env, "'%[^']s'", env_name) == 1)
            env += SDL_strlen(env_name) + 2;
        else if (SDL_sscanf(env, "%s", env_name) == 1)
            env += SDL_strlen(env_name);

        if (SDL_strcmp(name, env_name) == 0) {

            if (SDL_sscanf(env, "%d %d %d", &tmp_naxes, &tmp_nhats,
                           &tmp_nballs) == 3) {

                joystick->naxes = tmp_naxes;
                joystick->nhats = tmp_nhats;
                joystick->nballs = tmp_nballs;

                handled = SDL_TRUE;
            }
        }
    }

    /* Remap hats and balls */
    if (handled) {
        if (joystick->nhats > 0) {
            if (allocate_hatdata(joystick) < 0) {
                joystick->nhats = 0;
            }
        }
        if (joystick->nballs > 0) {
            if (allocate_balldata(joystick) < 0) {
                joystick->nballs = 0;
            }
        }
    }

    return (handled);
}

#if SDL_INPUT_LINUXEV

static SDL_bool
EV_ConfigJoystick(SDL_Joystick * joystick, int fd)
{
    int i, t;
    unsigned long keybit[NBITS(KEY_MAX)] = { 0 };
    unsigned long absbit[NBITS(ABS_MAX)] = { 0 };
    unsigned long relbit[NBITS(REL_MAX)] = { 0 };

    /* See if this device uses the new unified event API */
    if ((ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit) >= 0) &&
        (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absbit)), absbit) >= 0) &&
        (ioctl(fd, EVIOCGBIT(EV_REL, sizeof(relbit)), relbit) >= 0)) {
        joystick->hwdata->is_hid = SDL_TRUE;

        /* Get the number of buttons, axes, and other thingamajigs */
        for (i = BTN_JOYSTICK; i < KEY_MAX; ++i) {
            if (test_bit(i, keybit)) {
#ifdef DEBUG_INPUT_EVENTS
                printf("Joystick has button: 0x%x\n", i);
#endif
                joystick->hwdata->key_map[i - BTN_MISC] = joystick->nbuttons;
                ++joystick->nbuttons;
            }
        }
        for (i = BTN_MISC; i < BTN_JOYSTICK; ++i) {
            if (test_bit(i, keybit)) {
#ifdef DEBUG_INPUT_EVENTS
                printf("Joystick has button: 0x%x\n", i);
#endif
                joystick->hwdata->key_map[i - BTN_MISC] = joystick->nbuttons;
                ++joystick->nbuttons;
            }
        }
        for (i = 0; i < ABS_MISC; ++i) {
            /* Skip hats */
            if (i == ABS_HAT0X) {
                i = ABS_HAT3Y;
                continue;
            }
            if (test_bit(i, absbit)) {
                struct input_absinfo absinfo;

                if (ioctl(fd, EVIOCGABS(i), &absinfo) < 0)
                    continue;
#ifdef DEBUG_INPUT_EVENTS
                printf("Joystick has absolute axis: %x\n", i);
                printf("Values = { %d, %d, %d, %d, %d }\n",
                       absinfo.value, absinfo.minimum, absinfo.maximum,
                       absinfo.fuzz, absinfo.flat);
#endif /* DEBUG_INPUT_EVENTS */
                joystick->hwdata->abs_map[i] = joystick->naxes;
                if (absinfo.minimum == absinfo.maximum) {
                    joystick->hwdata->abs_correct[i].used = 0;
                } else {
                    joystick->hwdata->abs_correct[i].used = 1;
                    joystick->hwdata->abs_correct[i].coef[0] =
                        (absinfo.maximum + absinfo.minimum) / 2 - absinfo.flat;
                    joystick->hwdata->abs_correct[i].coef[1] =
                        (absinfo.maximum + absinfo.minimum) / 2 + absinfo.flat;
                    t = ((absinfo.maximum - absinfo.minimum) / 2 - 2 * absinfo.flat);
                    if (t != 0) {
                        joystick->hwdata->abs_correct[i].coef[2] =
                            (1 << 29) / t;
                    } else {
                        joystick->hwdata->abs_correct[i].coef[2] = 0;
                    }
                }
                ++joystick->naxes;
            }
        }
        for (i = ABS_HAT0X; i <= ABS_HAT3Y; i += 2) {
            if (test_bit(i, absbit) || test_bit(i + 1, absbit)) {
#ifdef DEBUG_INPUT_EVENTS
                printf("Joystick has hat %d\n", (i - ABS_HAT0X) / 2);
#endif
                ++joystick->nhats;
            }
        }
        if (test_bit(REL_X, relbit) || test_bit(REL_Y, relbit)) {
            ++joystick->nballs;
        }

        /* Allocate data to keep track of these thingamajigs */
        if (joystick->nhats > 0) {
            if (allocate_hatdata(joystick) < 0) {
                joystick->nhats = 0;
            }
        }
        if (joystick->nballs > 0) {
            if (allocate_balldata(joystick) < 0) {
                joystick->nballs = 0;
            }
        }
    }
    return (joystick->hwdata->is_hid);
}

#endif /* SDL_INPUT_LINUXEV */


/* Function to open a joystick for use.
   The joystick to open is specified by the index field of the joystick.
   This should fill the nbuttons and naxes fields of the joystick structure.
   It returns 0, or -1 if there is an error.
 */
int
SDL_SYS_JoystickOpen(SDL_Joystick * joystick, int device_index)
{
    int fd;
    char *fname;

    /* Open the joystick and set the joystick file descriptor */
    fd = open(SDL_joylist[joystick->instance_id].fname, O_RDONLY, 0);
    fname = SDL_joylist[joystick->instance_id].fname;

    if (fd < 0) {
        SDL_SetError("Unable to open %s\n", SDL_joylist[joystick->instance_id]);
        return (-1);
    }
    joystick->instance_id = device_index;
    joystick->hwdata = (struct joystick_hwdata *)
        SDL_malloc(sizeof(*joystick->hwdata));
    if (joystick->hwdata == NULL) {
        SDL_OutOfMemory();
        close(fd);
        return (-1);
    }
    SDL_memset(joystick->hwdata, 0, sizeof(*joystick->hwdata));
    joystick->hwdata->fd = fd;
    joystick->hwdata->fname = fname;

    /* Set the joystick to non-blocking read mode */
    fcntl(fd, F_SETFL, O_NONBLOCK);

    /* Get the number of buttons and axes on the joystick */
#if SDL_INPUT_LINUXEV
    if (!EV_ConfigJoystick(joystick, fd))
#endif
        JS_ConfigJoystick(joystick, fd);

    return (0);
}

/* Function to determine is this joystick is attached to the system right now */
SDL_bool SDL_SYS_JoystickAttached(SDL_Joystick *joystick)
{
    return SDL_TRUE;
}

static __inline__ void
HandleHat(SDL_Joystick * stick, Uint8 hat, int axis, int value)
{
    struct hwdata_hat *the_hat;
    const Uint8 position_map[3][3] = {
        {SDL_HAT_LEFTUP, SDL_HAT_UP, SDL_HAT_RIGHTUP},
        {SDL_HAT_LEFT, SDL_HAT_CENTERED, SDL_HAT_RIGHT},
        {SDL_HAT_LEFTDOWN, SDL_HAT_DOWN, SDL_HAT_RIGHTDOWN}
    };

    the_hat = &stick->hwdata->hats[hat];
    if (value < 0) {
        value = 0;
    } else if (value == 0) {
        value = 1;
    } else if (value > 0) {
        value = 2;
    }
    if (value != the_hat->axis[axis]) {
        the_hat->axis[axis] = value;
        SDL_PrivateJoystickHat(stick, hat,
                               position_map[the_hat->
                                            axis[1]][the_hat->axis[0]]);
    }
}

static __inline__ void
HandleBall(SDL_Joystick * stick, Uint8 ball, int axis, int value)
{
    stick->hwdata->balls[ball].axis[axis] += value;
}

/* Function to update the state of a joystick - called as a device poll.
 * This function shouldn't update the joystick structure directly,
 * but instead should call SDL_PrivateJoystick*() to deliver events
 * and update joystick device state.
 */
static __inline__ void
JS_HandleEvents(SDL_Joystick * joystick)
{
    struct js_event events[32];
    int i, len;
    Uint8 other_axis;

    while ((len = read(joystick->hwdata->fd, events, (sizeof events))) > 0) {
        len /= sizeof(events[0]);
        for (i = 0; i < len; ++i) {
            switch (events[i].type & ~JS_EVENT_INIT) {
            case JS_EVENT_AXIS:
                if (events[i].number < joystick->naxes) {
                    SDL_PrivateJoystickAxis(joystick,
                                            events[i].number,
                                            events[i].value);
                    break;
                }
                events[i].number -= joystick->naxes;
                other_axis = (events[i].number / 2);
                if (other_axis < joystick->nhats) {
                    HandleHat(joystick, other_axis,
                              events[i].number % 2, events[i].value);
                    break;
                }
                events[i].number -= joystick->nhats * 2;
                other_axis = (events[i].number / 2);
                if (other_axis < joystick->nballs) {
                    HandleBall(joystick, other_axis,
                               events[i].number % 2, events[i].value);
                    break;
                }
                break;
            case JS_EVENT_BUTTON:
                SDL_PrivateJoystickButton(joystick,
                                          events[i].number,
                                          events[i].value);
                break;
            default:
                /* ?? */
                break;
            }
        }
    }
}

#if SDL_INPUT_LINUXEV
static __inline__ int
EV_AxisCorrect(SDL_Joystick * joystick, int which, int value)
{
    struct axis_correct *correct;

    correct = &joystick->hwdata->abs_correct[which];
    if (correct->used) {
        if (value > correct->coef[0]) {
            if (value < correct->coef[1]) {
                return 0;
            }
            value -= correct->coef[1];
        } else {
            value -= correct->coef[0];
        }
        value *= correct->coef[2];
        value >>= 14;
    }

    /* Clamp and return */
    if (value < -32768)
        return -32768;
    if (value > 32767)
        return 32767;

    return value;
}

static __inline__ void
EV_HandleEvents(SDL_Joystick * joystick)
{
    struct input_event events[32];
    int i, len;
    int code;

    while ((len = read(joystick->hwdata->fd, events, (sizeof events))) > 0) {
        len /= sizeof(events[0]);
        for (i = 0; i < len; ++i) {
            code = events[i].code;
            switch (events[i].type) {
            case EV_KEY:
                if (code >= BTN_MISC) {
                    code -= BTN_MISC;
                    SDL_PrivateJoystickButton(joystick,
                                              joystick->hwdata->key_map[code],
                                              events[i].value);
                }
                break;
            case EV_ABS:
                if (code >= ABS_MISC) {
                    break;
                }

                switch (code) {
                case ABS_HAT0X:
                case ABS_HAT0Y:
                case ABS_HAT1X:
                case ABS_HAT1Y:
                case ABS_HAT2X:
                case ABS_HAT2Y:
                case ABS_HAT3X:
                case ABS_HAT3Y:
                    code -= ABS_HAT0X;
                    HandleHat(joystick, code / 2, code % 2, events[i].value);
                    break;
                default:
                    events[i].value =
                        EV_AxisCorrect(joystick, code, events[i].value);
                    SDL_PrivateJoystickAxis(joystick,
                                            joystick->hwdata->abs_map[code],
                                            events[i].value);
                    break;
                }
                break;
            case EV_REL:
                switch (code) {
                case REL_X:
                case REL_Y:
                    code -= REL_X;
                    HandleBall(joystick, code / 2, code % 2, events[i].value);
                    break;
                default:
                    break;
                }
                break;
            default:
                break;
            }
        }
    }
}
#endif /* SDL_INPUT_LINUXEV */

void
SDL_SYS_JoystickUpdate(SDL_Joystick * joystick)
{
    int i;

#if SDL_INPUT_LINUXEV
    if (joystick->hwdata->is_hid)
        EV_HandleEvents(joystick);
    else
#endif
        JS_HandleEvents(joystick);

    /* Deliver ball motion updates */
    for (i = 0; i < joystick->nballs; ++i) {
        int xrel, yrel;

        xrel = joystick->hwdata->balls[i].axis[0];
        yrel = joystick->hwdata->balls[i].axis[1];
        if (xrel || yrel) {
            joystick->hwdata->balls[i].axis[0] = 0;
            joystick->hwdata->balls[i].axis[1] = 0;
            SDL_PrivateJoystickBall(joystick, (Uint8) i, xrel, yrel);
        }
    }
}

/* Function to close a joystick after use */
void
SDL_SYS_JoystickClose(SDL_Joystick * joystick)
{
    if (joystick->hwdata) {
        close(joystick->hwdata->fd);
        if (joystick->hwdata->hats) {
            SDL_free(joystick->hwdata->hats);
        }
        if (joystick->hwdata->balls) {
            SDL_free(joystick->hwdata->balls);
        }
        SDL_free(joystick->hwdata);
        joystick->hwdata = NULL;
    }
    joystick->closed = 1;
}

/* Function to perform any system-specific joystick related cleanup */
void
SDL_SYS_JoystickQuit(void)
{
    int i;

    for (i = 0; SDL_joylist[i].fname; ++i) {
        if (SDL_joylist[i].fname) {
            SDL_free(SDL_joylist[i].fname);
            SDL_joylist[i].fname = NULL;
        }
    }
}

JoystickGUID SDL_SYS_JoystickGetDeviceGUID( int device_index )
{
    JoystickGUID guid;
    // the GUID is just the first 16 chars of the name for now
    const char *name = SDL_SYS_JoystickNameForDeviceIndex( device_index );
    SDL_zero( guid );
    SDL_memcpy( &guid, name, SDL_min( sizeof(guid), SDL_strlen( name ) ) );
    return guid;
}

JoystickGUID SDL_SYS_JoystickGetGUID(SDL_Joystick * joystick)
{
    JoystickGUID guid;
    // the GUID is just the first 16 chars of the name for now
    const char *name = joystick->name;
    SDL_zero( guid );
    SDL_memcpy( &guid, name, SDL_min( sizeof(guid), SDL_strlen( name ) ) );
    return guid;
}

#endif /* SDL_JOYSTICK_LINUX */

/* vi: set ts=4 sw=4 expandtab: */
