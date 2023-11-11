/*
 * Copyright (C) 2023 Microsoft / Neverball authors
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

#ifndef AUDIO_H
#define AUDIO_H

/*---------------------------------------------------------------------------*/

int audio_available(void);

void audio_init(void);
void audio_free(void);
void audio_suspend(void);
void audio_resume(void);

void audio_play(const char *, float);

void audio_narrator_play(const char *);

void audio_music_queue(const char *, float);
void audio_music_play(const char *);
void audio_music_stop(void);

void audio_music_fade_to(float, const char *);
//void audio_music_fade_to(float time, const char *file1, const char *file2);
void audio_music_fade_in(float);
void audio_music_fade_out(float);

void audio_volume(int, int, int, int);

/*---------------------------------------------------------------------------*/

#endif
