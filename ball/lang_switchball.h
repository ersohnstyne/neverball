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

#ifndef LANG_SWITCHBALL_H
#define LANG_SWITCHBALL_H

/*
 * This file were converted from XML to Poedit
 *
 * To compile single localization file on WSL2 or Ubuntu CLI, run:
 *     msgfmt --directory=./po <LANG_NAME>.po \
 *       --output-file=./locale/<LANG_NAME>/LC_MESSAGES/neverball.mo
 *
 * To test this language, run:
 *     make locales
 *
 * If you have an Msys2 environment for Windows installed, then run:
 *     mingw32-make locales
 */

#ifndef N_
/* No-op, useful for marking up strings for extraction-only. */
#define N_(s) s
#endif

#if NB_HAVE_PB_BOTH==1
#define SWITCHBALL_HAVE_TIP_AND_TUTORIAL
#endif

#define TIP_1      N_("Tip: Complete a level quickly to earn\nBronze, Silver, or Gold Medals!")
#define TIP_2      N_("Tip: Look out for Hidden Paths that can help you\nget through levels more quickly")
#define TIP_3      N_("Tip: To SHOOT from a Cannon, press the SPACE key.\nTo exit from a Cannon, press the ENTER key.")
#define TIP_3_PS4  N_("Tip: To SHOOT from a Cannon, press X button.\nTo exit from a Cannon, press Square button.")
#define TIP_3_XBOX N_("Tip: To SHOOT from a Cannon, press A button.\nTo exit from a Cannon, press B button.")
#define TIP_4      N_("Tip: Rotate the Manual Camera using\nthe LEFT and RIGHT MOUSE BUTTONS or\nzoom in and out using your MOUSE SCROLL WHEEL")
#define TIP_4_XBOX N_("Tip: Rotate the Manual Camera using\nthe RIGHT STICK or zoom in and out")
#define TIP_5      N_("Tip: Auto Camera selects the best position.\nManual Camera lets you control the camera.\nChase Camera turns behind your ball wherever you move.")
#define TIP_6      N_("Tip: Press the SPACE key to perform special ACTIONS\nsuch as Jump, Dash, or Shoot Cannon")
#define TIP_6_PS4  N_("Tip: Press X button to perform special ACTIONS\nsuch as Jump, Dash, or Shoot Cannon")
#define TIP_6_XBOX N_("Tip: Press A button to perform special ACTIONS\nsuch as Jump, Dash, or Shoot Cannon")
#define TIP_7      N_("Tip: For an extra boost, press and hold the\nLEFT MOUSE BUTTON, especially when climbing slopes")
#define TIP_8      N_("Tip: Keep your Mouse movements\nsmooth and steady for more control")

#define TUTORIAL_1_TITLE  N_("Controlling the Ball")
#define TUTORIAL_2_TITLE  N_("Max Speed")
#define TUTORIAL_3_TITLE  N_("Rotate Camera")
#define TUTORIAL_4_TITLE  N_("Zoom In / Out")
#define TUTORIAL_5_TITLE  N_("Switch")
#define TUTORIAL_6_TITLE  N_("Hold Switch")
#define TUTORIAL_7_TITLE  N_("Timer Switch")
#define TUTORIAL_8_TITLE  N_("Level Exit")
#define TUTORIAL_9_TITLE  N_("Checkpoint")
#define TUTORIAL_10_TITLE N_("Lift")

#define TUTORIAL_1_DESC                  N_("To move the Ball use your Mouse")
#define TUTORIAL_1_DESC_PS4              N_("To move the Ball, use the Left stick.\nIf you have enabled the SIXAXIS wireless\ncontroller motion sensor function,\nyou can move the Ball by tilting the controller.")
#define TUTORIAL_1_DESC_TOUCH            N_("To move the Ball, tap and hold the touchscreen")
#define TUTORIAL_1_DESC_TOUCH_GYROSCOPE  N_("To move the Ball, tap and hold the touchscreen.\nIf you have enabled the GYROSCOPE motion sensor function,\nyou can move the Ball by tilting the phone or tablet.")
#define TUTORIAL_1_DESC_WII              N_("To move the Ball, use with Nunchuck")
#define TUTORIAL_1_DESC_WIIU             N_("To move the Ball, tilt your Wii remote")
#define TUTORIAL_1_DESC_XBOX             N_("To move the Ball, use the Left stick")
#define TUTORIAL_2_DESC                  N_("Use the left Mouse Button to stay at full speed.\nThis will help you climb slopes.")
#define TUTORIAL_2_DESC_TOUCH            N_("Pull the touchscreen further to stay at full speed.\nThis will help you climb slopes.")
#define TUTORIAL_3_DESC                  N_("Hold down the right Mouse Button and move the Mouse\nfor free rotation of the camera")
#define TUTORIAL_3_DESC_TOUCH            N_("Tap and hold the rotate buttons\nfor free rotation of the camera")
#define TUTORIAL_3_DESC_WII              N_("Use the D-Pad to rotate the camera left or right")
#define TUTORIAL_3_DESC_XBOX             N_("Move the Right stick sideways\nto rotate the camera left or right")
#define TUTORIAL_4_DESC                  N_("Use the Mouse Scrollwheel\nto zoom the camera in and out")
#define TUTORIAL_4_DESC_TOUCH            N_("Tap and hold the zoom buttons\nto zoom the camera in and out")
#define TUTORIAL_4_DESC_WII              N_("Press D-Pad up or down\nto zoom the camera in and out")
#define TUTORIAL_4_DESC_XBOX             N_("Push the Right stick forwards or back\nto zoom the camera in and out")
#define TUTORIAL_5_DESC                  N_("Press Switches to turn machines\nOn or Off")
#define TUTORIAL_6_DESC                  N_("Some Switches must be held down\nto stay on")
#define TUTORIAL_7_DESC                  N_("Certain Switches will only remain on\nfor a limited time")
#define TUTORIAL_8_DESC                  N_("Board the Gyrocopter to complete the level")
#define TUTORIAL_9_DESC                  N_("Congratulations! You've reached a Checkpoint.\nYou will restart from here if you make a mistake.")
#define TUTORIAL_10_DESC                 N_("Floor Fans can give you a Lift")

#endif