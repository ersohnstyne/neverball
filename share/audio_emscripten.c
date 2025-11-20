/*
 * Copyright (C) 2024 Microsoft / Neverball Authors
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

#ifdef __EMSCRIPTEN__

#if NB_HAVE_PB_BOTH==1
#define FORCE_LOAD_MP3_MUSIC_FROM_WGCL
#endif

#include <emscripten.h>

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "config.h"
#include "audio.h"
#include "common.h"
#include "fs.h"
#include "log.h"
#include "strbuf/substr.h"
#include "strbuf/joinstr.h"

/*---------------------------------------------------------------------------*/

#define LOG_VOLUME(v) ((float) pow((double) (v), 2.0))

/*---------------------------------------------------------------------------*/

int audio_available(void)
{
    return 1;
}

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
    if (!filename || !filename[0])
    {
        log_errorf("filename returned 0!\n");
        return;
    }

#ifdef FORCE_LOAD_MP3_MUSIC_FROM_WGCL
    EM_ASM({
        Neverball.WGCLaudioPlay(UTF8ToString($0), $1);
    }, filename, a);
#else
#if NB_HAVE_PB_BOTH!=1
    int can_play_ogg = EM_ASM_INT({
        return Neverball.audioCanPlayOgg ? 1 : 0
    });
#endif

    int size = 0;
    unsigned char *data = NULL;

    if (!(filename && *filename))
        return;

    size_t len = strlen(filename);

#if NB_HAVE_PB_BOTH!=1
    if (can_play_ogg)
    {
        data = fs_load_cache(filename, &size);
    }
    else
#endif
    if (len > 3)
    {
        const char *mp3 = JOINSTR(SUBSTR(filename, 0, len - 3), "mp3");

        data = fs_load_cache(mp3, &size);
    }

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
    else
    {
        // Play game sound from the WGCL.

        EM_ASM({
            Neverball.WGCLaudioPlay(UTF8ToString($0), $1);
        }, filename, a);
    }
#endif
}

void audio_narrator_play(const char *filename)
{
    if (!filename || !filename[0])
    {
        log_errorf("filename returned 0!\n");
        return;
    }

#ifdef FORCE_LOAD_MP3_MUSIC_FROM_WGCL
    EM_ASM({
        Neverball.WGCLaudioNarratorPlay(UTF8ToString($0), $1);
    }, filename, 1.0f);
#else
#if NB_HAVE_PB_BOTH!=1
    int can_play_ogg = EM_ASM_INT({
        return Neverball.audioCanPlayOgg ? 1 : 0
    });
#endif

    int size = 0;
    unsigned char *data = NULL;

    if (!(filename && *filename))
        return;

    size_t len = strlen(filename);

#if NB_HAVE_PB_BOTH!=1
    if (can_play_ogg)
    {
        data = fs_load_cache(filename, &size);
    }
    else
#endif
    if (len > 3)
    {
        const char *mp3 = JOINSTR(SUBSTR(filename, 0, len - 3), "mp3");

        data = fs_load_cache(mp3, &size);
    }

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
        }, filename, data, size, 1.0f);
    }
    else
    {
        // Play narrator sound from the WGCL.

        EM_ASM({
            Neverball.WGCLaudioNarratorPlay(UTF8ToString($0), $1);
        }, filename, 1.0f);
    }
#endif
}

/*---------------------------------------------------------------------------*/

void audio_music_fade_out(float t)
{
    float clamped_time = CLAMP(0.001f, t, 1.0f);

    EM_ASM({
        Neverball.audioMusicFadeOut($0);
    }, clamped_time);
}

void audio_music_fade_in(float t)
{
    float clamped_time = CLAMP(0.001f, t, 1.0f);

    EM_ASM({
        Neverball.audioMusicFadeIn($0);
    }, clamped_time);
}

void audio_music_fade_to(float t, const char *filename, int loop)
{
    if (!filename || !filename[0])
    {
        log_errorf("filename returned 0!\n");
        return;
    }

    float clamped_time = CLAMP(0.001f, t, 1.0f);

#ifdef FORCE_LOAD_MP3_MUSIC_FROM_WGCL
    EM_ASM({
        Neverball.WGCLaudioMusicFadeTo(UTF8ToString($0), $1);
    }, filename, clamped_time);
#else
#if NB_HAVE_PB_BOTH!=1
    const int can_play_ogg = EM_ASM_INT({
        return Neverball.audioCanPlayOgg ? 1 : 0;
    });
#endif

    int size = 0;
    unsigned char *data = NULL;

    if (!(filename && *filename))
        return;

    size_t len = strlen(filename);

#if NB_HAVE_PB_BOTH!=1
    if (can_play_ogg)
    {
        data = fs_load_cache(filename, &size);
    }
    else
#endif
    if (len > 3)
    {
        const char *mp3 = JOINSTR(SUBSTR(filename, 0, len - 3), "mp3");

        data = fs_load_cache(mp3, &size);
    }

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
        }, filename, data, size, clamped_time);
    }
    else
    {
        // Play music from the WGCL.

        EM_ASM({
            Neverball.WGCLaudioMusicFadeTo(UTF8ToString($0), $1);
        }, filename, clamped_time);
    }
#endif
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
    }, master_vol, sound_vol, music_vol, narrator_vol);
}

/*---------------------------------------------------------------------------*/

#endif
