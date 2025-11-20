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

#ifndef ST_MALFUNCTION_H
#define ST_MALFUNCTION_H

/*
 * If there is more than 10 pressed without released keys,
 * an malfunction popup dialog will appear.
 */
#define MALFUNCTION_THRESHOLD 10

extern int malfunction_locked;
extern int char_downcounter[128];
extern int arrow_downcounter[5];
extern int fwindow_downcounter[13];

/*
 * HACK: Does not makes any sense
 *
 * extern struct state st_malfunction;
 * extern struct state st_handsoff;
 */

int check_malfunctions(void);
int check_handsoff(void);

int goto_handsoff(struct state *back);

#endif