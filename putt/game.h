/*
 * Copyright (C) 2023 Microsoft / Neverball authors
 *
 * PENNYPUTT is  free software; you can redistribute  it and/or modify
 * it under the  terms of the GNU General  Public License as published
 * by the Free  Software Foundation; either version 2  of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 */

#ifndef GAME_H
#define GAME_H

/*---------------------------------------------------------------------------*/

#define AUD_BIRDIE   _("snd/birdie.ogg")
#define AUD_BOGEY    _("snd/bogey.ogg")
#define AUD_BUMP       "snd/bink.ogg"
#define AUD_DOUBLE   _("snd/double.ogg")
#define AUD_EAGLE    _("snd/eagle.ogg")
#define AUD_JUMP       "snd/jump.ogg"
#define AUD_MENU       "snd/menu.ogg"
#define AUD_NEXTTURN   "snd/nextturn.ogg"
#define AUD_ONE      _("snd/one.ogg")
#define AUD_PAR      _("snd/par.ogg")
#define AUD_PENALTY  _("snd/penalty.ogg")
#define AUD_PLAYER1  _("snd/player1.ogg")
#define AUD_PLAYER2  _("snd/player2.ogg")
#define AUD_PLAYER3  _("snd/player3.ogg")
#define AUD_PLAYER4  _("snd/player4.ogg")
#define AUD_RETRY      "snd/rewind.ogg"
#define AUD_SWITCH     "snd/switch.ogg"
#define AUD_SUCCESS  _("snd/success.ogg")

/* Same as Neverball? */

#define AUD_BACK    "snd/back.ogg"

/*---------------------------------------------------------------------------*/

#define PUTT_GAMEMENU_ACTION(back_tok)                  \
    if (st_global_animating()) {                        \
        audio_play("snd/disabled.ogg", 1.f);            \
        return 1;                                       \
    }                                                   \
    audio_play(i == back_tok ? AUD_BACK : AUD_MENU, 1.f)

/*---------------------------------------------------------------------------*/

#define MAX_DT  (1.0f / 60.0f)         /* Maximum physics update cycle       */
#define MAX_DN  16                     /* Maximum subdivisions of dt         */
#define FOV     50.00f                 /* Field of view                      */
#define RESPONSE 0.05f                 /* Input smoothing time               */

#define GAME_NONE 0
#define GAME_STOP 1
#define GAME_GOAL 2
#define GAME_FALL 3

/*---------------------------------------------------------------------------*/

int   game_init(const char *);
void  game_free(void);

void  game_draw(int, float);
void  game_putt(void);
int   game_step(const float[3], float);

void  game_update_view(float);

void  game_set_rot(int);
void  game_clr_mag(void);
void  game_set_mag(int);
void  game_set_fly(float);

void  game_ball(int);
void  game_set_pos(float[3], float[3][3]);
void  game_get_pos(float[3], float[3][3]);

/*---------------------------------------------------------------------------*/

#endif
