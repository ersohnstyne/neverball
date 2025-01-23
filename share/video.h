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
#define FPS_REALTIME

#define ENABLE_MULTISAMPLE_SOLUTION

/*---------------------------------------------------------------------------*/

#if __cplusplus
extern const char TITLE[];
extern const char ICON[];
#endif

extern int opt_touch;
extern int video_has_touch;

struct video
{
    int device_w, device_h;
    int window_w, window_h;
    float scale_w, scale_h;

    float device_scale;
    float aspect_ratio;
};

extern struct video video;

#if ENABLE_MOTIONBLUR!=0
#define VIDEO_MOTIONBLUR_MAX_TEXTURE 8
#endif

extern int video_can_swap_window;

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

#if ENABLE_MOTIONBLUR!=0
void video_motionblur_init(void);
void video_motionblur_quit(void);

void  video_motionblur_alpha_set(float);
float video_motionblur_alpha_get(void);

void video_motionblur_prep(void);
void video_motionblur_swap(void);
#endif

/*---------------------------------------------------------------------------*/

void video_calc_view(float *, const float *,
                              const float *,
                              const float *);

/* Use video_set_perspective instead of video_push_persp. */
#define video_push_persp video_set_perspective

/* Use video_set_ortho instead of video_push_ortho. */
#define video_push_ortho video_set_ortho

/* This function is deprecated and is removed without notices. */
#define video_pop_matrix

void video_set_perspective(float, float, float);
void video_set_ortho(void);
void video_set_current(void);
void video_clear(void);

/*---------------------------------------------------------------------------*/

#endif
