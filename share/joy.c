/*
 * Copyright (C) 2025 Microsoft / Neverball authors / Jānis Rūcis
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
#include "dbg_config.h"

#if NB_PB_WITH_XBOX==0
#if _WIN32 && __MINGW32__
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_joystick.h>
#elif _WIN32 && _MSC_VER
#include <SDL.h>
#include <SDL_events.h>
#include <SDL_joystick.h>
#elif _WIN32
#error Security compilation error: No target include file in path for Windows specified!
#else
#include <SDL.h>
#include <SDL_events.h>
#include <SDL_joystick.h>
#endif
#endif

#include "console_control_gui.h"

#include "joy.h"
#include "state.h"
#include "common.h"
#include "log.h"

#if NB_PB_WITH_XBOX==0 && !defined(__GAMECUBE__) && !defined(__WII__)
#define JOY_MAX 16

#if _WIN32
static_assert(JOY_MAX == 4);
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

    GAMEDBG_SIGFUNC_PREPARE;

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
            log_printf("Failure to enable joystick (%s)\n",
                       GAMEDBG_GETSTRERROR_CHOICES_SDL);
        else
            joy_active_cursor(0, 1);
    }
    else
    {
        log_printf("Failure to initialize joystick (%s)\n",
                   GAMEDBG_GETSTRERROR_CHOICES_SDL);
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
            log_errorf("Failure to add Joystick! Joystick must be initialized!: %s\n",
                       GAMEDBG_GETSTRERROR_CHOICES_SDL);
#if _DEBUG
            SDL_TriggerBreakpoint();
#endif

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
                log_printf("Joystick %d made current via device addition\n",
                           joy_curr);

                break;
            }
        }

        if (i == ARRAYSIZE(joysticks))
        {
            SDL_JoystickClose(joy);
            log_errorf("Joystick %d not opened, %ud open joysticks reached\n",
                       device, ARRAYSIZE(joysticks));
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
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
        console_gui_toggle(1);
#endif

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
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
        console_gui_toggle(1);
#endif

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
            /*unsigned char gamepad_led[JOY_MAX][3] =
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
            */
            return 1;
        }
    }

    return 0;
}

#endif