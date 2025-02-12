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

#ifndef FETCH_H
#define FETCH_H

#define FETCH_MAX 5

/*
 * Progress callback extra_data.
 */
struct fetch_progress
{
    double total;
    double now;
};

/*
 * Done callback extra_data.
 */
struct fetch_done
{
    unsigned int success:1;
};

/*
 * Callbacks.
 */
struct fetch_callback
{
    void (*progress) (void *data, void *progress_data);
    void (*done)     (void *data, void *done_data);
    void *data;
};

extern unsigned long FETCH_EVENT;

void fetch_init(void);
void fetch_reinit(void);
void fetch_handle_event(void *);
void fetch_quit(void);

/*
 * This function will be replaced into the fetch_file_download.
 * Your functions will be replaced with same remaining parameters.
 */
#define fetch_file fetch_file_download

unsigned int fetch_file(const char *url,
                        const char *dst,
                        struct fetch_callback);

void fetch_enable(int enable);

#endif