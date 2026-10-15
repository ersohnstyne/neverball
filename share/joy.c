/*
 * Copyright (C) 2026 Microsoft / Neverball authors / Jānis Rūcis
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
#if NB_HAVE_PB_BOTH==1 && NB_PB_SDL3==1
#define SDL_ENABLE_OLD_NAMES
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_joystick.h>
#include <SDL3/SDL_oldnames.h>
#elif _WIN32 && __MINGW32__
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
static_assert(JOY_MAX == 4, "JOY_MAX: For Xbox, Nintendo Switch or Playstation: This must be set to 4!");
#endif

static int joy_is_init = 0;

static struct
{
    SDL_GameController *gamectrlr;
    SDL_Joystick       *joy;

    SDL_JoystickID id;
} joysticks[JOY_MAX];

int joy_curr_active[JOY_MAX];
static SDL_JoystickID joy_curr = -1;

/*
 * Initialize joystick state.
 */
int joy_init(void)
{
    if (joy_is_init) return 1;

    GAMEDBG_SIGFUNC_PREPARE;

    size_t i = 0;
    unsigned int init_flags = SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER;
    //unsigned int init_flags = SDL_INIT_JOYSTICK;

#if NB_HAVE_PB_BOTH==1 && NB_PB_SDL3==1
    if (SDL_InitSubSystem(init_flags))
#else
    if (SDL_InitSubSystem(init_flags) == 0)
#endif
    {
        for (i = 0; i < ARRAYSIZE(joysticks); ++i)
        {
            joysticks[i].gamectrlr = NULL;
            joysticks[i].joy       = NULL;
            joysticks[i].id        = -1;
        }
#if NB_HAVE_PB_BOTH==1 && NB_PB_SDL3!=1
        joy_is_init = SDL_JoystickEventState(SDL_ENABLE) == 1;

        if (joy_is_init < 0)
            log_printf("Failure to enable joystick (%s)\n",
                       GAMEDBG_GETSTRERROR_CHOICES_SDL);
        else
#else
        joy_is_init = 1;
#endif
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
    if (!joy_is_init) return;

    size_t i = 0;

    for (i = 0; i < ARRAYSIZE(joysticks); ++i)
        if (joysticks[i].joy || joysticks[i].gamectrlr)
            joy_remove(joysticks[i].id);

    SDL_QuitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
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
            log_errorf("Failure to add Joystick! Joystick and GameController must be initialized!: %s\n",
                       GAMEDBG_GETSTRERROR_CHOICES_SDL);
#if _DEBUG
            SDL_TriggerBreakpoint();
#endif

            return;
        }
    }

#if NB_HAVE_PB_BOTH==1 && NB_PB_SDL3==1
    SDL_Joystick *joy = SDL_OpenJoystick(device);
#else
    SDL_Joystick *joy = SDL_JoystickOpen(device);
#endif

    if (joy != NULL)
    {
        log_printf("Joystick added (device %d)\n", device);

        size_t i = 0;

        for (i = 0; i < ARRAYSIZE(joysticks); ++i)
        {
            if (joysticks[i].joy == NULL)
            {
                joysticks[i].joy = joy;
#if NB_HAVE_PB_BOTH==1 && NB_PB_SDL3==1
                joysticks[i].id = SDL_GetJoystickID(joy);
#else
                joysticks[i].id = SDL_JoystickInstanceID(joy);
#endif
                log_printf("Joystick opened (instance %d)\n", joysticks[i].id);

                joy_curr = joysticks[i].id;
                log_printf("Joystick %d made current via device addition\n",
                           joy_curr);

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
                console_gui_toggle(1);
#endif
                break;
            }
        }

        if (i == ARRAYSIZE(joysticks))
        {
#if NB_HAVE_PB_BOTH==1 && NB_PB_SDL3==1
            SDL_CloseJoystick(joy);
#else
            SDL_JoystickClose(joy);
#endif
            log_errorf("Joystick %d not opened, %ud open GameControllers or joysticks reached\n",
                       device, ARRAYSIZE(joysticks));
        }
    }
}

/*
 * Handle joystick add event (GameController).
 */
void joy_add_gamectrlr(int device)
{
    if (!joy_is_init)
    {
        if (!joy_init())
        {
            log_errorf("Failure to add GameController! Joystick and GameController must be initialized!: %s\n",
                       GAMEDBG_GETSTRERROR_CHOICES_SDL);
#if _DEBUG
            SDL_TriggerBreakpoint();
#endif

            return;
        }
    }

#if NB_HAVE_PB_BOTH==1 && NB_PB_SDL3==1
    SDL_Gamepad *gamectrlr = SDL_OpenGamepad(device);
#else
    SDL_GameController *gamectrlr = SDL_GameControllerOpen(device);
#endif

    if (gamectrlr != NULL)
    {
        log_printf("Joystick added (device %d) (GameController)\n", device);

        size_t i = 0;

        for (i = 0; i < ARRAYSIZE(joysticks); ++i)
        {
            if (joysticks[i].gamectrlr == NULL)
            {
                joysticks[i].gamectrlr = gamectrlr;
#if NB_HAVE_PB_BOTH==1 && NB_PB_SDL3==1
                joysticks[i].id = SDL_GetJoystickID(SDL_GetGamepadJoystick(gamectrlr));
#else
                joysticks[i].id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gamectrlr));
#endif
                log_printf("GameController opened (instance %d)\n", joysticks[i].id);

                joy_curr = joysticks[i].id;
                log_printf("GameController %d made current via device addition\n",
                           joy_curr);

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
                console_gui_toggle(1);
#endif
                break;
            }
        }

        if (i == ARRAYSIZE(joysticks))
        {
#if NB_HAVE_PB_BOTH==1 && NB_PB_SDL3==1
            SDL_CloseGamepad(gamectrlr);
#else
            SDL_GameControllerClose(gamectrlr);
#endif
            log_errorf("GameController %d not opened, %ud open GameControllers or joysticks reached\n",
                       device, ARRAYSIZE(joysticks));
        }
    }
}

/*
 * Handle joystick remove event.
 */
void joy_remove(int instance)
{
    if (!joy_is_init) return;

    size_t i;

    if (instance > -1 && instance < JOY_MAX && joysticks[instance].joy)
        log_printf("Joystick removed (instance %d)\n", instance);
    else if (instance > -1 && instance < JOY_MAX && joysticks[instance].gamectrlr)
        log_printf("GameController removed (instance %d)\n", instance);

    for (i = 0; i < ARRAYSIZE(joysticks); ++i)
    {
        if (joysticks[i].id == instance)
        {
            if (joysticks[i].joy) {
#if NB_HAVE_PB_BOTH==1 && NB_PB_SDL3==1
                SDL_CloseJoystick(joysticks[i].joy);
#else
                SDL_JoystickClose(joysticks[i].joy);
#endif
                joysticks[i].joy = NULL;
                log_printf("Joystick closed (instance %d)\n", instance);
            }

            if (joysticks[i].gamectrlr) {
#if NB_HAVE_PB_BOTH==1 && NB_PB_SDL3==1
                SDL_CloseGamepad(joysticks[i].gamectrlr);
#else
                SDL_GameControllerClose(joysticks[i].gamectrlr);
#endif
                joysticks[i].gamectrlr = NULL;
                log_printf("GameController closed (instance %d)\n", instance);
            }

            joysticks[i].id = -1;
        }
    }
}

/*
 * Handle joystick button event.
 */
int joy_button(int instance, int b, int d)
{
    if (!joy_is_init) return 1;

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    console_gui_toggle(1);
#endif

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
 * Handle gamepad button event.
 */
int joy_button_gamectrlr(int instance, int b, int d)
{
    if (!joy_is_init) return 1;

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    console_gui_toggle(1);
#endif

    if (joy_curr != instance)
    {
        /* Make gamepad current. */
#if NEVERBALL_FAMILY_API != NEVERBALL_PC_FAMILY_API
        joy_curr = instance;
        //log_printf("Joystick %d made current via button press\n", joy_curr);
#endif
    }

    static int pad[4] = { 0, 0, 0, 0 };

    switch (b)
    {
        case SDL_CONTROLLER_BUTTON_A:             return st_buttn(0, d); break;
        case SDL_CONTROLLER_BUTTON_B:             return st_buttn(1, d); break;
        case SDL_CONTROLLER_BUTTON_X:             return st_buttn(2, d); break;
        case SDL_CONTROLLER_BUTTON_Y:             return st_buttn(3, d); break;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:  return st_buttn(4, d); break;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return st_buttn(5, d); break;
        case SDL_CONTROLLER_BUTTON_BACK:          return st_buttn(8, d); break;
        case SDL_CONTROLLER_BUTTON_START:         return st_buttn(9, d); break;

        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  pad[0] = d; return st_dpad(8,  d, pad); break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: pad[1] = d; return st_dpad(9,  d, pad); break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP:    pad[2] = d; return st_dpad(10, d, pad); break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  pad[3] = d; return st_dpad(11, d, pad); break;
    }

    return 1;
}

/*
 * Handle joystick axis event.
 */
void joy_axis(int instance, int a, float v)
{
    if (!joy_is_init) return;

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
 * Handle gamepad axis event.
 */
void joy_axis_gamectrlr(int instance, int a, float v)
{
    if (!joy_is_init) return;

    if (joy_curr == instance)
    {
        int d = 1;

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
        console_gui_toggle(1);
#endif

        /* Process axis events from current gamepad only. */
        
        switch (a)
        {
            case SDL_CONTROLLER_AXIS_LEFTX:            st_stick(0, v); break;
            case SDL_CONTROLLER_AXIS_LEFTY:            st_stick(1, v); break;
            case SDL_CONTROLLER_AXIS_RIGHTX:           st_stick(3, v); break;
            case SDL_CONTROLLER_AXIS_RIGHTY:           st_stick(4, v); break;
            case SDL_CONTROLLER_AXIS_TRIGGERLEFT:  d = st_buttn(6, v > 0.1f); break;
            case SDL_CONTROLLER_AXIS_TRIGGERRIGHT: d = st_buttn(7, v > 0.1f); break;
        }

        if (!d) {
            SDL_Event e = { SDL_QUIT };
            SDL_PushEvent(&e);
        }
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
        if (joysticks[instance].id != -1 && joysticks[instance].joy)
        {
            const unsigned char gamepad_led[JOY_MAX][3] = {
                { 0xff, 0x00, 0x00 },
                { 0x00, 0xff, 0x00 },
                { 0x00, 0x00, 0xff },
                { 0xff, 0xff, 0x00 },
                /*{ 0xbf, 0x00, 0xff },
                { 0xbf, 0x00, 0x00 },
                { 0x00, 0xbf, 0x00 },
                { 0x00, 0x00, 0xbf },
                { 0xbf, 0xbf, 0x00 },
                { 0x80, 0x00, 0xbf }*/
            };
            
#if NB_HAVE_PB_BOTH==1 && NB_PB_SDL3==1
#if !defined(__EMSCRIPTEN__)
#if NEVERBALL_FAMILY_API == NEVERBALL_ENERGIZELAB_FAMILY_API || \
    NEVERBALL_FAMILY_API == NEVERBALL_PS_FAMILY_API || \
    NEVERBALL_FAMILY_API == NEVERBALL_STEAMDECK_FAMILY_API
            if (instance <= JOY_MAX)
                SDL_SetJoystickLED(joysticks[instance].joy, 0x00, 0xbf, 0xff);
#else
            if (instance <= JOY_MAX)
                SDL_SetJoystickLED(joysticks[instance].joy, 0x00, 0xff, 0xff);
#endif
            else
                SDL_SetJoystickLED(joysticks[instance].joy, 0x80, 0x00, 0x00);
#endif

            int out_pct = -1;
            SDL_PowerState power_state = SDL_GetGamepadPowerInfo(joysticks[instance].gamectrlr, &out_pct);

            if (battery_level) {
                if (out_pct <    0) (*battery_level) = -1;
                if (out_pct <=   5) (*battery_level) =  0;
                if (out_pct <=  20) (*battery_level) =  1;
                if (out_pct <=  70) (*battery_level) =  2;
                if (out_pct <= 100) (*battery_level) =  3;
            }

            if (wired)
                (*wired) = (power_state == SDL_POWERSTATE_NO_BATTERY ||
                            power_state == SDL_POWERSTATE_CHARGING   ||
                            power_state == SDL_POWERSTATE_CHARGED) ? 1 : 0;
#else
#if !defined(__EMSCRIPTEN__)
#if NEVERBALL_FAMILY_API == NEVERBALL_ENERGIZELAB_FAMILY_API || \
    NEVERBALL_FAMILY_API == NEVERBALL_PS_FAMILY_API || \
    NEVERBALL_FAMILY_API == NEVERBALL_STEAMDECK_FAMILY_API
            if (instance <= JOY_MAX)
                SDL_JoystickSetLED(joysticks[instance].joy, 0x00, 0xbf, 0xff);
#else
            if (instance <= JOY_MAX)
                SDL_JoystickSetLED(joysticks[instance].joy, 0x00, 0xff, 0x00);
#endif
            else
                SDL_JoystickSetLED(joysticks[instance].joy, 0x80, 0x00, 0x00);
#endif

            SDL_JoystickPowerLevel power_level = SDL_JoystickCurrentPowerLevel(joysticks[instance].joy);

            if (battery_level)
                if (power_level <= 3) (*battery_level) = power_level;

            if (wired)
                (*wired) = power_level == SDL_JOYSTICK_POWER_WIRED;
#endif

            return 1;
        }
    }

    return 0;
}

#endif