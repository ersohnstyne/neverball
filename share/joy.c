/*
 * Copyright (C) 2023 Microsoft / Neverball authors
 *
 * NEVERBALL is  free software; you can redistribute  it and/or modify
 * it under the  terms of the GNU General  Public License as published
 * by the Free  Software Foundation; either version 2  of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 */

#include "base_config.h"

#if NB_PB_WITH_XBOX==0
#if _WIN32 && __MINGW32__
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_joystick.h>
#else
#include <SDL.h>
#include <SDL_events.h>
#include <SDL_hints.h>
#include <SDL_joystick.h>
#endif
#endif

#include "joy.h"
#include "state.h"
#include "common.h"
#include "log.h"

#if NB_PB_WITH_XBOX==0
#if NEVERBALL_FAMILY_API == NEVERBALL_PC_FAMILY_API
#define JOY_MAX 10 // Default: 16
#else
/*
 * HACK: On Xbox, Playstation or Nintendo Switch, this causes
 * only limited number of gamepads.
 */
#define JOY_MAX 4
#endif

static int joy_is_init = 0;

static struct
{
    SDL_Joystick *joy;
    SDL_JoystickID id;
} joysticks[JOY_MAX];

int joy_curr_active[JOY_MAX];
static SDL_JoystickID joy_curr = -1;

/*
 * Initialize joystick state.
 */
int joy_init(void)
{
    if (joy_is_init)
        return 1;

    size_t i = 0;

    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) == 0)
    {
        for (i = 0; i < ARRAYSIZE(joysticks); ++i)
        {
            joysticks[i].joy = NULL;
            joysticks[i].id = -1;
        }

        joy_is_init = SDL_JoystickEventState(SDL_ENABLE) == 1;

        if (joy_is_init < 0)
            log_printf("Failure to enable joystick (%s)\n", SDL_GetError() ? SDL_GetError() : "Unknown error");
        else
            joy_active_cursor(0, 1);
    }
    else
    {
        log_printf("Failure to initialize joystick (%s)\n", SDL_GetError() ? SDL_GetError() : "Unknown error");
        return 0;
    }

    return joy_is_init;
}

/*
 * Close all joysticks.
 */
void joy_quit(void)
{
    if (!joy_is_init)
        return;

    size_t i = 0;

    for (i = 0; i < ARRAYSIZE(joysticks); ++i)
        if (joysticks[i].joy)
            joy_remove(joysticks[i].id);

    SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}

/*
 * Handle joystick add event.
 */
void joy_add(int device)
{
    if (!joy_is_init)
    {
        if (!joy_init())
        {
            log_errorf("Failure to add Joystick! Joystick must be initialized!: %s\n", SDL_GetError() ? SDL_GetError() : "Unknown error");
            exit(1);
            return;
        }
    }

    log_printf("Joystick added (device %d)\n", device);

    SDL_Joystick *joy = SDL_JoystickOpen(device);

    if (joy != NULL)
    {
        size_t i = 0;

        for (i = 0; i < ARRAYSIZE(joysticks); ++i)
        {
            if (joysticks[i].joy == NULL)
            {
                joysticks[i].joy = joy;
                joysticks[i].id = SDL_JoystickInstanceID(joy);
                log_printf("Joystick opened (instance %d)\n", joysticks[i].id);

                joy_curr = joysticks[i].id;
                log_printf("Joystick %d made current via device addition\n", joy_curr);

                break;
            }
        }

        if (i == ARRAYSIZE(joysticks))
        {
            SDL_JoystickClose(joy);
            log_errorf("Joystick %d not opened, %ud open joysticks reached\n", device, ARRAYSIZE(joysticks));
        }
    }
}

/*
 * Handle joystick remove event.
 */
void joy_remove(int instance)
{
    if (!joy_is_init)
        return;

    size_t i;

    if (instance > -1)
        log_printf("Joystick removed (instance %d)\n", instance);

    for (i = 0; i < ARRAYSIZE(joysticks); ++i)
    {
        if (joysticks[i].id == instance)
        {
            SDL_JoystickClose(joysticks[i].joy);
            joysticks[i].joy = NULL;
            joysticks[i].id = -1;
            log_printf("Joystick closed (instance %d)\n", instance);
        }
    }
}

/*
 * Handle joystick button event.
 */
int joy_button(int instance, int b, int d)
{
    if (!joy_is_init)
        return 1;

    if (joy_curr != instance)
    {
        /* Make joystick current. */
#if NEVERBALL_FAMILY_API != NEVERBALL_PC_FAMILY_API
        joy_curr = instance;
        //log_printf("Joystick %d made current via button press\n", joy_curr);
#endif
    }

    /* Process button event normally. */
    return st_buttn(b, d);
}

/*
 * Handle joystick axis event.
 */
void joy_axis(int instance, int a, float v)
{
    if (!joy_is_init)
        return;

    if (joy_curr == instance)
    {
        /* Process axis events from current joystick only. */
        st_stick(a, v);
    }
}

/*
 * Set active player gamepad index.
 * @param Player index: 0 - {JOY_MAX}
 */
void joy_active_cursor(int instance, int active)
{
    joy_curr = instance;

    if (instance >= 0 && instance < JOY_MAX)
        joy_curr_active[instance] = active;
}

int  joy_get_active_cursor(int instance)
{
    if (instance >= 0 && instance < JOY_MAX)
        return joy_curr_active[instance];

    return 0;
}

int  joy_get_gamepad_action_index(void)
{
    return joy_curr;
}

int  joy_get_cursor_actions(int instance)
{
    if (instance >= 0 && instance < JOY_MAX)
        return joy_curr_active[instance] && joy_curr_active[joy_curr];

    return 0;
}

/*
 * Check player gamepad connected.
 * @param Player index: 0 - {JOY_MAX}
 */
int  joy_connected(int instance, int *battery_level, int *wired)
{
    if (wired) (*wired) = 0;
    if (battery_level) (*battery_level) = 3;

    if (instance >= 0 && instance < JOY_MAX)
    {
        if (joysticks[instance].id != -1)
        {
            unsigned char gamepad_led[JOY_MAX][3] =
            {
                { 0xff, 0x00, 0x00 },
                { 0x00, 0xff, 0x00 },
                { 0x00, 0x00, 0xff },
                { 0xff, 0xff, 0x00 },
                { 0xbf, 0x00, 0xff },
                { 0xbf, 0x00, 0x00 },
                { 0x00, 0xbf, 0x00 },
                { 0x00, 0x00, 0xbf },
                { 0xbf, 0xbf, 0x00 },
                { 0x80, 0x00, 0xbf }
            };

#if NEVERBALL_FAMILY_API == NEVERBALL_PC_FAMILY_API || \
    NEVERBALL_FAMILY_API == NEVERBALL_PS_FAMILY_API
            if (instance <= 4)
                SDL_JoystickSetLED(joysticks[instance].id, 0x00, 0xbf, 0xff);
            else
                SDL_JoystickSetLED(joysticks[instance].id, 0x80, 0x00, 0x00);
#endif

            if (battery_level)
                (*battery_level) = SDL_JoystickCurrentPowerLevel(joysticks[instance].joy);

            return 1;
        }
    }

    return 0;
}

#endif