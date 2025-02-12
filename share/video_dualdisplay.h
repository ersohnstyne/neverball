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

#ifndef VIDEO_DUALDISPLAY_H
#define VIDEO_DUALDISPLAY_H

struct video_dualdisplay
{
    int ddpy_device_w, ddpy_device_h;
    int ddpy_window_w, ddpy_window_h;
    float ddpy_scale_w, ddpy_scale_h;

    float ddpy_device_scale;
    float ddpy_aspect_ratio;
};

extern struct video_dualdisplay video_ddpy;

int  video_dualdisplay_init(void);
void video_dualdisplay_quit(void);

int  video_dualdisplay_is_init(void);

/*---------------------------------------------------------------------------*/

int  video_dualdisplay_mode(int, int, int);

void video_dualdisplay_snap(const char *);
void video_dualdisplay_swap(void);

int  video_dualdisplay_fullscreen(int);
void video_dualdisplay_resize(int, int);
void video_dualdisplay_set_window_size(int, int);
void video_dualdisplay_set_display(int);
int  video_dualdisplay_display(void);

/*---------------------------------------------------------------------------*/

void video_dualdisplay_set_current(void);
void video_dualdisplay_clear(void);

/*---------------------------------------------------------------------------*/

#endif
