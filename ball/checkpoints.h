#ifndef CHECKPOINTS_H
#define CHECKPOINTS_H

#include "solid_chkp.h"

#ifdef MAPC_INCLUDES_CHKP

struct chkp_ballsize
{
    float size_orig;
    int size_state;
};

struct chkp_ball
{
    float p[3];
    float r;
};

struct chkp_path
{
    int f;
};

struct chkp_move
{
    float t;
    int tm;

    int pi;
};

struct chkp_item
{
    float p[3];
    int t;
    int n;
};

struct chkp_swch
{
    float t;
    int   tm;
    int   f;
};

extern int last_active;
extern int checkpoints_busy;

extern struct chkp_ballsize last_chkp_ballsize[1024];
extern struct chkp_ball last_chkp_ball[1024];
extern struct chkp_path last_chkp_path[2048];
extern struct chkp_move last_chkp_move[1024];
extern struct chkp_item last_chkp_item[2048];
extern struct chkp_swch last_chkp_swch[1024];
extern struct game_view last_view[1024];

extern float last_pos[1024][3];

extern float last_time;
extern int last_coins;
extern int last_goal;

extern int last_timer_down;
extern int last_gained;

extern int respawn_coins;
extern float respawn_timer;

/*---------------------------------------------------------------------------*/

void checkpoints_save_spawnpoint(struct s_vary,
                                 struct game_view,
                                 int);

void checkpoints_respawn(void);
void checkpoints_respawn_done(void);

int checkpoints_respawn_coins(void);
int checkpoints_respawn_timer(void);

/*
 * Please disable all checkpoints,
 * before you attempt to restart the level.
 */
void checkpoints_stop(void);

void checkpoints_set_last_data(float, int, int);

#endif
#endif
