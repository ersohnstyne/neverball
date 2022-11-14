#ifndef STATE_H
#define STATE_H

#if _MSC_VER
#define _CRT_NB_SCREENSTATE_DEPRECATED(_ItemReplacement)                  \
    __declspec(deprecated(                                                \
        "This screenstate function or variable has been superceded by "   \
        "newer library functionality. Consider using " #_ItemReplacement  \
        " instead."                                                       \
    ))
#else
#define _CRT_NB_SCREENSTATE_DEPRECATED(_ItemReplacement)                  \
    __attribute__((deprecated(                                            \
        "This screenstate function or variable has been superceded by "   \
        "newer library functionality. Consider using " #_ItemReplacement  \
        " instead."                                                       \
    )))
#endif

#if _WIN32 && __GNUC__
#include <SDL2/SDL_events.h>
#else
#include <SDL_events.h>
#endif

/*---------------------------------------------------------------------------*/

/*
 * Things throttles, just find the keyword regex
 * with "struct state [^*]+= {\n"
 */

struct state
{
    int  (*enter) (struct state *, struct state *prev);
    void (*leave) (struct state *, struct state *next, int id);
    void (*paint) (int id, float t);
    void (*timer) (int id, float dt);
    void (*point) (int id, int x, int y, int dx, int dy);
    void (*stick) (int id, int a, float v, int bump);
    void (*angle) (int id, float x, float z);
    int  (*click) (int b,  int d);
    int  (*keybd) (int c,  int d);
    int  (*buttn) (int b,  int d);
    void (*wheel) (int x,  int y);
    int  (*touch) (const SDL_TouchFingerEvent*);
    void (*fade)  (float alpha);

    int gui_id;
};

struct state *curr_state(void);

float time_state(void);
void  init_state(struct state *);

/*
 * This screenstate transition will be replaced into the goto_state_full.
 * Your functions will be replaced using four parameters.
 */
#if _MSC_VER && !__GNUC__
_CRT_NB_SCREENSTATE_DEPRECATED(goto_state_full)
int  goto_state(struct state *st);
#else
int  goto_state(struct state* st)
     _CRT_NB_SCREENSTATE_DEPRECATED(goto_state_full);
#endif

int  goto_state_full(struct state *st, int fromdirection, int todirection, int noanimation);

void st_paint(float, int);
void st_timer(float);
void st_point(int, int, int, int);
void st_stick(int, float);
void st_angle(float, float);
void st_wheel(int, int);
int  st_click(int, int);
int  st_keybd(int, int);
int  st_buttn(int, int);
int  st_touch(const SDL_TouchFingerEvent*);

/*---------------------------------------------------------------------------*/

#endif
