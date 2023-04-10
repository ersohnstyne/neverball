#ifndef ST_TUTORIAL_H
#define ST_TUTORIAL_H

/* HINTS */

#define HINT_BUNNY_SLOPE 1
#define HINT_HAZARDOUS_CLIMB 2
#define HINT_HALFPIPE 3
#define HINT_UPWARD_SPIRAL 4

#define HINT_BUNNY_SLOPE_TEXT "Build up momentum to reach the bottom quickly!"
#define HINT_HAZARDOUS_CLIMB_TEXT "Don't let these hazards\\throw you off the track to finish!"
#define HINT_HALFPIPE_TEXT "Speed around the half pipe and collect the coins!"
#define HINT_UPWARD_SPIRAL_TEXT "Climb upward to reach the goal!"

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