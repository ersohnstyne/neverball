/*
 * Copyright (C) 2024 Microsoft / Neverball Authors
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

#ifdef __EMSCRIPTEN__

#include <emscripten.h>

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "config.h"
#include "audio.h"
#include "common.h"
#include "fs.h"
#include "log.h"

/*---------------------------------------------------------------------------*/

#define LOG_VOLUME(v) ((float) pow((double) (v), 2.0))

/*---------------------------------------------------------------------------*/

void audio_init(void)
{
    /* Initialize web audio. */

    EM_ASM({
        Neverball.audioInit();
    });

    /* Set the initial volumes. */
    
    audio_volume(CLAMP(0, config_get_d(CONFIG_MASTER_VOLUME),   10),
                 CLAMP(0, config_get_d(CONFIG_SOUND_VOLUME),    10),
                 CLAMP(0, config_get_d(CONFIG_MUSIC_VOLUME),    10),
                 CLAMP(0, config_get_d(CONFIG_NARRATOR_VOLUME), 10));
}

void audio_free(void)
{
    EM_ASM({
        Neverball.audioQuit();
    });
}

void audio_play(const char *filename, float a)
{
    int size = 0;
    unsigned char *data = fs_load_cache(filename, &size);

    if (data)
    {
        // Play the file data.

        EM_ASM({
            const fileName = UTF8ToString($0);
            const data = $1;
            const size = $2;
            const a = $3;

            const fileData = Module.HEAP8.buffer.slice(data, data + size);

            Neverball.audioPlay(fileName, fileData, a);
        }, filename, data, size, LOG_VOLUME(CLAMP(0.0f, a, 1.0f)));
    }
}

void audio_narrator_play(const char *filename, float a)
{
    int size = 0;
    unsigned char *data = fs_load_cache(filename, &size);

    if (data)
    {
        // Play the file data.

        EM_ASM({
            const fileName = UTF8ToString($0);
            const data = $1;
            const size = $2;
            const a = $3;

            const fileData = Module.HEAP8.buffer.slice(data, data + size);

            Neverball.audioPlayNarrator(fileName, fileData, a);
        }, filename, data, size, LOG_VOLUME(CLAMP(0.0f, a, 1.0f)));
    }
}

/*---------------------------------------------------------------------------*/

void audio_music_fade_out(float t)
{
    EM_ASM({
        Neverball.audioMusicFadeOut($0);
    }, t);
}

void audio_music_fade_in(float t)
{
    EM_ASM({
        Neverball.audioMusicFadeIn($0);
    }, t);
}

void audio_music_fade_to(float t, const char *filename)
{
    int size = 0;
    unsigned char *data = fs_load_cache(filename, &size);

    if (data)
    {
        // Play the file data.

        EM_ASM({
            const fileName = UTF8ToString($0);
            const data = $1;
            const size = $2;
            const t = $3;

            const fileData = Module.HEAP8.buffer.slice(data, data + size);

            Neverball.audioMusicFadeTo(fileName, fileData, t);
        }, filename, data, size, t);
    }
}

void audio_music_stop(void)
{
    EM_ASM({
        Neverball.audioMusicStop();;
    });
}

/*---------------------------------------------------------------------------*/

/*
 * Logarithmic volume control.
 */
void audio_volume(int master, int sound, int music, int narrator)
{
    float master_logaritmic   = (float)  master   / 10.0f;
    float sound_logaritmic    = (float) (sound    / 10.0f) * master_logaritmic;
    float music_logaritmic    = (float) (music    / 10.0f) * master_logaritmic;
    float narrator_logaritmic = (float) (narrator / 10.0f) * master_logaritmic;

    float master_vol   = LOG_VOLUME(master_logaritmic);
    float sound_vol    = LOG_VOLUME(sound_logaritmic);
    float music_vol    = LOG_VOLUME(music_logaritmic);
    float narrator_vol = LOG_VOLUME(narrator_logaritmic);

    EM_ASM({
        Neverball.audioVolume($0, $1, $2, $3);
    }, master_logaritmic, sound_vol, music_vol, narrator_vol);
}

/*---------------------------------------------------------------------------*/

#endif
