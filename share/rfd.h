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

#ifndef RFD_H
#define RFD_H

#include "base_config.h"

/*---------------------------------------------------------------------------*/

/* Integer options. */

extern int RFD_CAMPAIGN_MILES_PERCENTAGE;
extern int RFD_SET_MILES_PERCENTAGE;
extern int RFD_CHALLENGE_MILES_PERCENTAGE;
extern int RFD_CHALLENGE_BALLS;
extern int RFD_CHALLENGE_EARNINATOR;
extern int RFD_CHALLENGE_FLOATIFIER;
extern int RFD_CHALLENGE_SPEEDIFIER;

/* String options. */

/* TODO: Add RFD extern string options here. */

/*---------------------------------------------------------------------------*/

void rfd_init(void);
void rfd_quit(void);
void rfd_load(void);

/*---------------------------------------------------------------------------*/

void rfd_set_d(int, int);
int  rfd_get_d(int);

/*---------------------------------------------------------------------------*/

#endif
