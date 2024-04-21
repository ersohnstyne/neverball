/*
 * Copyright (C) 2024 Microsoft / Neverball authors
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

#ifndef ST_TUTORIAL_H
#define ST_TUTORIAL_H

/* HINTS */

/* MB Beginner - Bunny Slope > Classic Medium 9 */
#define HINT_BUNNY_SLOPE_TEXT \
    N_("Build up momentum to reach the bottom quickly!")
/* MB Beginner - Hazardous Climb > ? */
#define HINT_HAZARDOUS_CLIMB_TEXT \
    N_("Don't let these hazards\n" \
       "throw you off the track to finish!")
/* MB Beginner - Pitfalls > Classic Easy 10 */
#define HINT_PITFALLS_TEXT \
    N_("Practice your rolling skills\n" \
       "by avoiding the gaps in the floor!")
/* MB Beginner - Platform Party > ? */
#define HINT_PLATFORM_PARTY \
    N_("Ride the moving platforms to reach the finish!")
/* MB Beginner - Half-Pipe > Classic Med 13 */
#define HINT_HALF_PIPE_TEXT \
    N_("Speed around the half pipe and collect the coins!")
/* MB Beginner - Upward Spiral > FWP 6 */
#define HINT_UPWARD_SPIRAL_TEXT \
    N_("Climb upward to reach the goal!")
/* MB Advanced - Survival of the fittest > Tdf 2 */
#define HINT_SURVIVAL_FITTEST \
    N_("Stay on the platform to survive!")
/* MB Intermediate - Fork in the road > Tones 16 */
#define HINT_FORKROAD_TEXT \
    N_("See how quickly you can collect all coins!")
/* MB Advanced - Endurance > Tdf 2 / Tdf V */
#define HINT_ENDURANCE_TEXT \
    N_("Don't fall behind the platform\n" \
       "- there's no going back!")
/* MB Advanced - Under Construction > FWP 11 */
#define HINT_UNDERCONSTRUCTION_TEXT \
    N_("Be very cautious on this framework.")

/* END HINTS */

/*---------------------------------------------------------------------------*/

int tutorial_check(void);
int goto_tutorial(int);
int goto_tutorial_before_play(int);

int hint_check(void);
int goto_hint(int);
int goto_hint_before_play(int);

extern struct state st_tutorial;
extern struct state st_hint;

#endif