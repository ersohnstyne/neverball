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

#include "joy.h"
#include "config.h"
#include "state.h"
#include "common.h"
#include "log.h"
#include "vec3.h"

/*
 * HACK: Have it your way on your Xbox Gamepads from Visual Studio 2022!
 * - Ersohn Styne
 */

#if NB_PB_WITH_XBOX==1
#pragma comment(lib, "xinput.lib")

/*
 * HACK: On Xbox, Playstation or Nintendo Switch, this causes
 * only limited number of gamepads.
 */
#define JOY_MAX XUSER_MAX_COUNT

typedef struct _XboxJoysticks
{
    XINPUT_STATE curr_state;
    XINPUT_STATE prev_state;

    XINPUT_BATTERY_INFORMATION battery_info;

    int id;
} XboxJoysticks, *PXBoxJoysticks;

static XboxJoysticks joysticks[JOY_MAX];

int joy_curr_active[JOY_MAX];
static int joy_curr = 0;

/*
 * Initialize joystick state.
 */
int joy_init(void)
{
    XInputEnable(1);

    /* HACK: This can be done with fixed gamepad state. */

    for (int i = 0; i < JOY_MAX; i++)
    {
        memset(&joysticks[i].battery_info, 0, sizeof(XINPUT_BATTERY_INFORMATION));
        memset(&joysticks[i].curr_state, 0, sizeof(XINPUT_STATE));
        memset(&joysticks[i].prev_state, 0, sizeof(XINPUT_STATE));
        joysticks[i].id = -1;
    }

    joy_active_cursor(0, 1);
    
    return 1;
}

/*
 * Close all joysticks.
 */
void joy_quit(void)
{
    for (int i = 0; i < ARRAYSIZE(joysticks); ++i)
        joy_remove(joysticks[i].id);

    XInputEnable(0);
}

/*
 * Handle joystick add event.
 */
void joy_add(int device)
{
    log_printf("Joystick added (device %d)\n", device);

    size_t i = 0;

    for (i = 0; i < ARRAYSIZE(joysticks); ++i)
    {
        if (joysticks[i].id != device && joysticks[i].id == -1)
        {
            joysticks[i].id = device;
            log_printf("Joystick opened (instance %d)\n", joysticks[i].id);

            break;
        }
    }

    if (i == ARRAYSIZE(joysticks))
        log_errorf("Joystick %d not opened, %ud open joysticks reached\n", device, ARRAYSIZE(joysticks));
}

/*
 * Handle joystick remove event.
 */
void joy_remove(int instance)
{
    size_t i;

    if (instance > -1)
        log_printf("Joystick removed (instance %d)\n", instance);

    for (i = 0; i < ARRAYSIZE(joysticks); ++i)
    {
        if (joysticks[i].id == instance && joysticks[i].id != -1)
        {
            memset(&joysticks[i].battery_info, 0, sizeof(XINPUT_BATTERY_INFORMATION));
            memset(&joysticks[i].curr_state, 0, sizeof(XINPUT_STATE));
            memset(&joysticks[i].prev_state, 0, sizeof(XINPUT_STATE));
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
    if (joy_curr != instance)
    {
        /* Make joystick current. */
        
        joy_curr = instance;

        /* log_printf("Joystick %d made current via button press\n", joy_curr); */
    }

    /* Process button event. */
    return st_buttn(b, d);
}

/*
 * Handle joystick axis event.
 */
void joy_axis(int instance, int a, float v)
{
    if (joy_curr != instance)
    {
        /* Make joystick current. */

        joy_curr = instance;

        /* log_printf("Joystick %d made current via button press\n", joy_curr); */
    }

    // if (joy_curr == instance)
    {
        /* Process axis events. */
        st_stick(a, v);
    }
}

/*
 * Set active player gamepad index.
 * @param Player index: 0 - 3
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
 * @param Player index: 0 - 3
 */
int  joy_connected(int instance, int *battery_level, int *wired)
{
    if (wired) (*wired) = 1;
    if (battery_level) (*battery_level) = 3;

    if (instance >= 0 && instance < JOY_MAX)
    {
        if (wired)
        {
            (*wired) = joysticks[instance].battery_info.BatteryType & BATTERY_TYPE_WIRED;
            if (battery_level) (*battery_level) = 3;
        }
        if (battery_level)
            (*battery_level) = joysticks[instance].battery_info.BatteryLevel;

        return joysticks[instance].id != -1;
    }

    return 0;
}

void joy_update(void)
{
    DWORD result;

    for (int i = 0; i < JOY_MAX; i++)
    {
        result = XInputGetState(i, &joysticks[i].curr_state);

        if (result == ERROR_SUCCESS)
        {
            XInputGetBatteryInformation(i, BATTERY_DEVTYPE_GAMEPAD, &joysticks[i].battery_info);

            if (joysticks[i].id == -1)
                joy_add(i);

            PXINPUT_STATE currState = &joysticks[i].curr_state;
            PXINPUT_STATE prevState = &joysticks[i].prev_state;

            if (memcmp(prevState, currState, sizeof(*currState)) != 0)
            {
                float currLX = currState->Gamepad.sThumbLX;
                float currLY = currState->Gamepad.sThumbLY;
                float prevLX = prevState->Gamepad.sThumbLX;
                float prevLY = prevState->Gamepad.sThumbLY;
                float currRX = currState->Gamepad.sThumbRX;
                float currRY = currState->Gamepad.sThumbRY;
                float prevRX = prevState->Gamepad.sThumbRX;
                float prevRY = prevState->Gamepad.sThumbRY;

                if (prevState->Gamepad.wButtons != currState->Gamepad.wButtons)
                {
                    joy_button(i, config_get_d(CONFIG_JOYSTICK_BUTTON_A),
                               currState->Gamepad.wButtons & XINPUT_GAMEPAD_A);

                    joy_button(i, config_get_d(CONFIG_JOYSTICK_BUTTON_B),
                               currState->Gamepad.wButtons & XINPUT_GAMEPAD_B);

                    joy_button(i, config_get_d(CONFIG_JOYSTICK_BUTTON_X),
                               currState->Gamepad.wButtons & XINPUT_GAMEPAD_X);

                    joy_button(i, config_get_d(CONFIG_JOYSTICK_BUTTON_Y),
                               currState->Gamepad.wButtons & XINPUT_GAMEPAD_Y);

                    joy_button(i, config_get_d(CONFIG_JOYSTICK_BUTTON_L1),
                               currState->Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);

                    joy_button(i, config_get_d(CONFIG_JOYSTICK_BUTTON_R1),
                               currState->Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);

                    joy_button(i, config_get_d(CONFIG_JOYSTICK_BUTTON_START),
                               currState->Gamepad.wButtons & XINPUT_GAMEPAD_START);

                    joy_button(i, config_get_d(CONFIG_JOYSTICK_BUTTON_SELECT),
                               currState->Gamepad.wButtons & XINPUT_GAMEPAD_BACK);
                }

                if (prevState->Gamepad.bLeftTrigger != currState->Gamepad.bLeftTrigger)
                    joy_button(i, config_get_d(CONFIG_JOYSTICK_BUTTON_L2),
                               currState->Gamepad.bLeftTrigger > 15);
                if (prevState->Gamepad.bRightTrigger != currState->Gamepad.bRightTrigger)
                    joy_button(i, config_get_d(CONFIG_JOYSTICK_BUTTON_R2),
                               currState->Gamepad.bRightTrigger > 15);

                float magnitudeL = fsqrtf(currLX * currLX + currLY * currLY);
                float magnitudeR = fsqrtf(currRX * currRX + currRY * currRY);

                float normalizedLX = currLX / magnitudeL;
                float normalizedLY = currLY / magnitudeL;
                float normalizedRX = currRX / magnitudeR;
                float normalizedRY = currRY / magnitudeR;

                if (prevLX != currLX || prevLY != currLY)
                {
                    if (magnitudeL > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
                    {
                        joy_axis(i, config_get_d(CONFIG_JOYSTICK_AXIS_X0), normalizedLX);
                        joy_axis(i, config_get_d(CONFIG_JOYSTICK_AXIS_Y0), normalizedLY);
                    }
                    else
                    {
                        joy_axis(i, config_get_d(CONFIG_JOYSTICK_AXIS_X0), 0);
                        joy_axis(i, config_get_d(CONFIG_JOYSTICK_AXIS_Y0), 0);
                    }
                }

                if (prevRX != currRX || prevRY != currRY)
                {
                    if (magnitudeR > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
                    {
                        joy_axis(i, config_get_d(CONFIG_JOYSTICK_AXIS_X1), normalizedRX);
                        joy_axis(i, config_get_d(CONFIG_JOYSTICK_AXIS_Y1), normalizedRY);
                    }
                    else
                    {
                        joy_axis(i, config_get_d(CONFIG_JOYSTICK_AXIS_X1), 0);
                        joy_axis(i, config_get_d(CONFIG_JOYSTICK_AXIS_Y1), 0);
                    }
                }

                memcpy(prevState, currState, sizeof(*currState));
            }
        }
        else if (joysticks[i].id != -1)
            joy_remove(joysticks[i].id);
    }
}

#endif
