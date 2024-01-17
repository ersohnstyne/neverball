/*
 * Copyright (C) 2024 Microsoft / Neverball authors
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

#ifndef COURSE_H
#define COURSE_H

/*---------------------------------------------------------------------------*/

#define COURSE_FILE "courses.txt"
#define MAXCRS 16

/*---------------------------------------------------------------------------*/

void course_init(void);
void course_free(void);

int  course_exists(int);
int  course_count(void);
void course_goto(int);
int  course_curr(void);
void course_rand(void);

const char *course_name(int);
const char *course_desc(int);
const char *course_shot(int);

/*---------------------------------------------------------------------------*/

#endif
