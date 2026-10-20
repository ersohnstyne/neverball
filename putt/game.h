/*
 * Copyright (C) 2026 Microsoft / Neverball authors / Jānis Rūcis
 *
 * NEVERPUTT is  free software; you can redistribute  it and/or modify
 * it under the  terms of the GNU General  Public License as published
 * by the Free  Software Foundation; either version 2  of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 */

#ifndef PUTT_H
#define PUTT_H

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

#define MAX_DT  (1.0f / 90.0f)         /* Maximum physics update cycle       */
#define MAX_DN  16                     /* Maximum subdivisions of dt         */
#define FOV     50.00f                 /* Field of view                      */
#define RESPONSE 0.05f                 /* Input smoothing time               */

#define PUTT_NONE 0
#define PUTT_STOP 1
#define PUTT_GOAL 2
#define PUTT_FALL 3

/*---------------------------------------------------------------------------*/

int  putt_init(const char *);
void putt_free(void);
void putt_free_objects(void);
int  putt_load_objects(void);

void putt_kill_fade(void);
void putt_step_fade(float);
void putt_fade(float d);

void putt_draw(int, float);
void putt_putt(void);
int  putt_step(const float[3], float);

void putt_update_view(float);

void putt_set_rot(int);
void putt_clr_mag(void);
void putt_set_mag(int);
void putt_set_fly(float);

void putt_ball(int);
void putt_set_pos(float[3], float[3][3]);
void putt_get_pos(float[3], float[3][3]);

void putt_set_fast(int);

/*---------------------------------------------------------------------------*/

#endif
