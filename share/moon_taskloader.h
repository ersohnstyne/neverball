/*
 * Copyright (C) 2026 Microsoft / Neverball authors / Ersohn Styne
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

#ifndef MOON_TASKLOADER_H
#define MOON_TASKLOADER_H

struct moon_taskloader_execute
{
    char *filename;
    int   time_ms;
    int   goal_enabled;
};

struct moon_taskloader_progress
{
    double total;
    double now;
};

struct moon_taskloader_done
{
    unsigned int success:1;
};

struct moon_taskloader_callback
{
    int  (*execute)  (void *data, void *execute_data);
    void (*progress) (void *data, void *progress_data);
    void (*done)     (void *data, void *done_data);
    void *data;
};

struct game_moon_taskloader_info
{
    struct moon_taskloader_callback callback;
    char                           *filename;
};

void moon_taskloader_init(void (*dispatch_event) (void *));
void moon_taskloader_handle_event(void *);
void moon_taskloader_quit(void);

unsigned int moon_taskloader_load(const char *,
                                  struct moon_taskloader_callback);

#endif
