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

#ifndef VIDEO_H
#define VIDEO_H

/* Don't use the build-in resolutions */
#define RESIZEABLE_WINDOW

/*---------------------------------------------------------------------------*/

#if __cplusplus
extern const char TITLE[];
extern const char ICON[];
#endif

struct video
{
    int device_w, device_h;
    int window_w, window_h;
    float scale_w, scale_h;

    float device_scale;
    float aspect_ratio;
};

extern struct video video;

extern int viewport_wireframe;
extern int wireframe_splitview;
extern int splitview_crossed;

extern int render_fill_overlay;
extern int render_line_overlay;
extern int render_left_viewport;
extern int render_right_viewport;

int  video_init(void);
void video_quit(void);

/*---------------------------------------------------------------------------*/

int  video_mode(int, int, int);

/* Switchball uses auto configuration */
int  video_mode_auto_config(int, int, int);

void video_snap(const char *);
int  video_perf(void);
void video_swap(void);

void video_show_cursor(void);
void video_hide_cursor(void);

void video_set_grab(int w);
void video_clr_grab(void);
int  video_get_grab(void);

int  video_fullscreen(int);
void video_resize(int, int);
void video_set_window_size(int, int);
void video_set_display(int);
int  video_display(void);

/*---------------------------------------------------------------------------*/

void video_set_wire(int);
void video_render_fill_or_line(int);
void video_toggle_wire(void);

void video_calc_view(float *, const float *,
                              const float *,
                              const float *);

void video_set_perspective(float, float, float);
void video_set_ortho(void);
void video_clear(void);

/*---------------------------------------------------------------------------*/

#endif
