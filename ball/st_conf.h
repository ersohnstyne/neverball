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

#ifndef ST_CONF_H
#define ST_CONF_H

#include "state.h"

extern int conf_covid_extended;
extern int online_mode;

extern struct state st_conf_account;
extern struct state st_conf;
extern struct state st_null;

extern struct state st_conf_video;
extern struct state st_conf_display;

int  goto_conf(struct state *, int, int);
void conf_covid_retract(void);

#endif
