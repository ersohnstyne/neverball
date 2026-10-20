<<<<<<< HEAD
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

=======
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b
#ifndef ST_ALL_H
#define ST_ALL_H

#include "state.h"

extern struct state st_putt_title;
<<<<<<< HEAD
extern struct state st_putt_party;
extern struct state st_putt_next;
extern struct state st_putt_over;

int goto_putt_pause(int);
=======
extern struct state st_putt_course;
extern struct state st_putt_party;
extern struct state st_putt_next;
extern struct state st_putt_poser;
extern struct state st_putt_flyby;
extern struct state st_putt_stroke;
extern struct state st_putt_roll;
extern struct state st_putt_goal;
extern struct state st_putt_stop;
extern struct state st_putt_fall;
extern struct state st_putt_score;
extern struct state st_putt_over;
extern struct state st_putt_pause;

int goto_putt_pause(struct state *);
>>>>>>> 86bbf940e14a5a1f26e7aa7172d3e7f6938a3a1b

#endif
