#ifndef STATE_H
#define STATE_H

#include "SDL_events.h"

/*---------------------------------------------------------------------------*/

struct state
{
    int  (*enter)(struct state *, struct state *prev, int intent);
    int  (*leave)(struct state *, struct state *next, int id, int intent);
    void (*paint)(int id, float t);
    void (*timer)(int id, float dt);
    void (*point)(int id, int x, int y, int dx, int dy);
    void (*stick)(int id, int a, float v, int bump);
    void (*angle)(int id, float x, float z);
    int  (*click)(int b,  int d);
    int  (*keybd)(int c,  int d);
    int  (*buttn)(int b,  int d);
    void (*wheel)(int x,  int y);

    int gui_id;

    int  (*touch)(const SDL_TouchFingerEvent *);
};

struct state *curr_state(void);

float time_state(void);
void  init_state(struct state *);
int   goto_state(struct state *);
int   exit_state(struct state *);

void st_paint(float);
void st_timer(float);
void st_point(int, int, int, int);
void st_stick(int, float);
void st_angle(float, float);
void st_wheel(int, int);
int  st_click(int, int);
int  st_keybd(int, int);
int  st_buttn(int, int);
int  st_touch(const SDL_TouchFingerEvent *);

/*---------------------------------------------------------------------------*/

#endif
