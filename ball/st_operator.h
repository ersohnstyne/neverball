/*
 * Copyright (C) 2026 Microsoft / Neverball authors / Jānis Rūcis
 *
 * PENNYBALL is  free software; you can redistribute  it and/or modify
 * it under the  terms of the GNU General  Public License as published
 * by the Free  Software Foundation; either version 2  of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 */

#ifndef ST_OPERATOR_H
#define ST_OPERATOR_H

#if (_WIN32 && _MSC_VER) && NB_HAVE_PB_BOTH==1

void demo_operator_init     (void);
void demo_operator_quit     (void);
int  demo_operator_activated(void);

int  demo_operator_map_name(const char *);

int goto_operator_search(void);

int operator_send_incident_from_replay(int,
                                       float, float, float);
#endif

#endif