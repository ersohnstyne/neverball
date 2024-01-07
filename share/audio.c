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

#include <assert.h>

#if _WIN32 && __MINGW32__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif

#define OV_EXCLUDE_STATIC_CALLBACKS
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#include <string.h>
#include <stdlib.h>

#include "dbg_config.h"

#include "accessibility.h"
#include "config.h"
#include "audio.h"
#include "common.h"
#include "fs.h"
#include "fs_ov.h"
#include "log.h"

#if _DEBUG && _MSC_VER
#ifndef _CRTDBG_MAP_ALLOC
#pragma message(__FILE__": Missing CRT-Debugger include header, recreate: crtdbg.h")
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#endif

#ifdef _MSC_VER
#pragma comment(lib, "libogg_static.lib")
#pragma comment(lib, "libvorbis_static.lib")
#pragma comment(lib, "libvorbisfile_static.lib")
#endif

/*---------------------------------------------------------------------------*/

#define AUDIO_RATE 44100
#define AUDIO_CHAN 2

struct voice
{
    OggVorbis_File  vf;
    float          amp;
    float         damp;
    int           chan;
    int           play;
    int           loop;
    char         *name;
    struct voice *next;
};

/*
 * SDL3 uses only for SDL_LockAudioStream(), neither SDL_LockAudioDevice() nor
 * SDL_UnlockAudioDevice().
 * - Ersohn Styne
 */
static int   lock_hold;
static int   speed_freezes;

/*
 * SDL3 uses only for SDL_PauseAudioDevice() and SDL_ResumeAudioDevice().
 * - Ersohn Styne
 */
static int   audio_paused    = 0;
static int   audio_state     = 0;
static int   audio_device_id = -1;

static float master_vol   = 1.0f;
static float sound_vol    = 1.0f;
static float music_vol    = 1.0f;
static float narrator_vol = 1.0f;

static SDL_AudioSpec spec;

/* HACK: Have fun using AudioDevice for MSVC++ */
static SDL_AudioSpec device_spec;

static struct voice *music     = NULL;
static struct voice *queue     = NULL;
static struct voice *voices    = NULL;
static struct voice *narrators = NULL;
static short        *buffer    = NULL;

static ov_callbacks callbacks = {
    fs_ov_read, fs_ov_seek, fs_ov_close, fs_ov_tell
};

static int   has_multiple_music;
static char *next_music_filename;

/*---------------------------------------------------------------------------*/

#define LOGF_VOLUME(v) ((float) powf((v), 2.f))
#define LOG_VOLUME(v)  ((float) pow((double) (v), 2.0))

#define MIX(d, s) {                           \
        int T = (int) (d) + (int) (s);        \
        if      (T >  32767) (d) =  32767;    \
        else if (T < -32768) (d) = -32768;    \
        else                 (d) = (short) T; \
    }

static int voice_step(struct voice *V, float volume, Uint8 *stream, int length)
{
    if (audio_paused) return 0;

    int order = 0;

    short *obuf = (short *) stream;
    char  *ibuf = (char  *) buffer;

    int i, b = 0, n = 1, c = 0, r = 0;

    /* Compute the total request size for the current stream. */

    if (V->chan == 1) r = length / 2;
    if (V->chan == 2) r = length    ;

    /* While data is coming in and data is still needed... */

    while (n > 0 && r > 0)
    {
        /* Read audio from the stream. */

        if ((n = (int) ov_read(&V->vf, ibuf, r, order, 2, 1, &b)) > 0)
        {
            /* Mix mono audio. */

            if (V->chan == 1)
                for (i = 0; i < n / 2; i += 1)
                {
                    short M = (short) (LOGF_VOLUME(V->amp) * volume * buffer[i]);

                    MIX(obuf[c], M); c++;
                    MIX(obuf[c], M); c++;

                    V->amp += V->damp;
                    V->amp = CLAMP(0, V->amp + V->damp, 1);
                }

            /* Mix stereo audio. */

            if (V->chan == 2)
                for (i = 0; i < n / 2; i += 2)
                {
                    short L = (short) (LOGF_VOLUME(V->amp) * volume * buffer[i + 0]);
                    short R = (short) (LOGF_VOLUME(V->amp) * volume * buffer[i + 1]);

                    MIX(obuf[c], L); c++;
                    MIX(obuf[c], R); c++;

                    V->amp += V->damp;
                    V->amp = CLAMP(0, V->amp + V->damp, 1);
                }

            r -= n;
        }
        else
        {
            /* We're at EOF.  Loop or end the voice. */

            if (V->loop)
            {
                ov_raw_seek(&V->vf, 0);
                n = 1;
            }
            else return 1;
        }
    }

    return 0;
}

static struct voice *voice_init(const char *filename, float a)
{
    struct voice *V;
    fs_file      fp;

    /* Allocate and initialize a new voice structure. */

    if ((V = (struct voice *) calloc(1, sizeof (struct voice))))
    {
        /* Note the name. */

        V->name = strdup(filename);

        /* Attempt to open the named Ogg stream. */

        if ((fp = fs_open_read(filename)))
        {
            if (ov_open_callbacks(fp, &V->vf, NULL, 0, callbacks) == 0)
            {
                vorbis_info *info = ov_info(&V->vf, -1);

                /* On success, configure the voice. */

                V->amp  = a;
                V->damp = 0;
                V->chan = info->channels;
                V->play = 1;
                V->loop = 0;

                if (V->amp > 1.0f) V->amp = 1.0;
                if (V->amp < 0.0f) V->amp = 0.0;

                /* The file will be closed when the Ogg is cleared. */
            }
            else fs_close(fp);
        }
    }
    return V;
}

static void voice_free(struct voice *V)
{
    ov_clear(&V->vf);

    free(V->name); V->name = NULL;
    free(V);
}

static void voice_quit(struct voice* V)
{
    if (V->next)
    {
        voice_quit(V->next);
        V->next = NULL;
    }

    voice_free(V);
}

/*---------------------------------------------------------------------------*/

static void audio_step(void *data, Uint8 *stream, int length)
{
    struct voice *V = voices;
    struct voice *N = narrators;
    struct voice *P = NULL;

    /* Zero the output buffer. */

    memset(stream, 0, length);

    /* Mix the background music. */

    if (music)
    {
        voice_step(music, music_vol, stream, length);

        /* If the track has faded out, move to a queued track. */

        if (music && music->amp <= 0.0f && music->damp < 0.0f && queue)
        {
            voice_free(music);
            music = queue;
            queue = NULL;
        }
    }

    /* Iterate over all active voices. */

    while (V)
    {
        /* Mix this voice. */

        if (V->play && voice_step(V, sound_vol, stream, length))
        {
            /* Delete a finished voice... */

            struct voice *T1 = V;

            if (P)
                V = P->next = V->next;
            else
                V = voices  = V->next;

            voice_free(T1);
        }
        else
        {
            /* ... or continue to the next. */

            P = V;
            V = V->next;
        }
    }

    /* Iterate over narrator active voices. */

    while (N)
    {
        /* Mix this voice. */

        if (N->play && voice_step(N, narrator_vol, stream, length))
        {
            /* Delete a finished voice... */

            struct voice *T2 = N;

            if (P)
                N = P->next   = N->next;
            else
                N = narrators = N->next;

            //voice_free(T2);
        }
        else
        {
            /* ... or continue to the next. */

            P = N;
            N = N->next;
        }
    }
}

/*---------------------------------------------------------------------------*/

int audio_available(void)
{
    return audio_state;
}

/*---------------------------------------------------------------------------*/

void audio_init(void)
{
    if (audio_state) return;
    if (accessibility_get_d(ACCESSIBILITY_SLOWDOWN) < 40)
        return;

    /* Configure the audio. */

    memset(&device_spec, 0, sizeof (device_spec));
    device_spec.format   = AUDIO_S16;
    device_spec.channels = AUDIO_CHAN;
    device_spec.samples  = config_get_d(CONFIG_AUDIO_BUFF);
    device_spec.freq     = AUDIO_RATE * (float) (accessibility_get_d(ACCESSIBILITY_SLOWDOWN) / 100.f);
    device_spec.callback = audio_step;

    memset(&spec, 0, sizeof (spec));
    spec.format   = AUDIO_S16;
    spec.channels = AUDIO_CHAN;
    spec.samples  = config_get_d(CONFIG_AUDIO_BUFF);
    spec.freq     = AUDIO_RATE * (float) (accessibility_get_d(ACCESSIBILITY_SLOWDOWN) / 100.f);
    spec.callback = audio_step;

    /* Allocate an input buffer. */

    if ((buffer = (short *) malloc(spec.samples * 4)))
    {
        /**
         * Start the audio thread.
         */

        audio_device_id = -1;
        if ((audio_device_id = SDL_OpenAudioDevice(
            0,
            0,
            &spec,
            &device_spec,
            0
        )) > 0)
        {
            audio_state = 1;

            audio_paused = 0;

            SDL_PauseAudioDevice(audio_device_id, 0);
        }
        else
            log_errorf("Failure to open audio device (%s)\n", GAMEDBG_GETSTRERROR_CHOICES_SDL);
    }

    if (audio_state == 0)
    {
        free(buffer);
        buffer = NULL;

        return;
    }

    /* Set the initial volumes. */

    int assert_master   = config_get_d(CONFIG_MASTER_VOLUME);
    int assert_snd      = config_get_d(CONFIG_SOUND_VOLUME);
    int assert_mus      = config_get_d(CONFIG_MUSIC_VOLUME);
    int assert_narrator = config_get_d(CONFIG_NARRATOR_VOLUME);

    assert(assert_master   >= 0 &&
           assert_snd      >= 0 &&
           assert_mus      >= 0 &&
           assert_narrator >= 0 &&
           "Values cannot be negative");

    audio_volume(config_get_d(CONFIG_MASTER_VOLUME),
                 config_get_d(CONFIG_SOUND_VOLUME),
                 config_get_d(CONFIG_MUSIC_VOLUME),
                 config_get_d(CONFIG_NARRATOR_VOLUME));
}

void audio_free(void)
{
    if (!audio_state) return;

    /* Halt the audio thread. */

    if (audio_state)
        SDL_CloseAudioDevice(audio_device_id);

    audio_device_id = -1;

    /* Release the input buffer. */

    audio_music_stop();

    if (voices)
    {
        voice_quit(voices);
        voices = NULL;
    }

    if (narrators)
    {
        voice_quit(narrators);
        narrators = NULL;
    }

    if (buffer)
    {
        free(buffer);
        buffer = NULL;
    }

    /* Ogg streams and voice structure remain open to allow quality setting. */

    audio_state = 0;
}

void audio_suspend(void)
{
    if (audio_paused) return;

    audio_paused = 1;
}

void audio_resume(void)
{
    if (!audio_paused) return;

    audio_paused = 0;
}

/*---------------------------------------------------------------------------*/

void audio_play(const char *filename, float a)
{
    if (audio_state && !audio_paused)
    {
        while (lock_hold) {}
        struct voice *V;

        /* If we're already playing this sound, preempt the running copy. */

        lock_hold = 1;
        {
            for (V = voices; V; V = V->next)
                if (strcmp(V->name, filename) == 0)
                {
                    ov_raw_seek(&V->vf, 0);

                    V->amp = a;

                    if (V->amp > 1.0f) V->amp = 1.0;
                    if (V->amp < 0.0f) V->amp = 0.0;

                    lock_hold = 0;
                    return;
                }
        }
        lock_hold = 0;

        /* Create a new voice structure. */

        V = voice_init(filename, a);

        /* Add it to the list of sounding voices. */

        lock_hold = 1;
        {
            V->next = voices;
            voices  = V;
        }
        lock_hold = 0;
    }
    else if (!audio_state && !audio_paused)
        log_errorf("Failure to open audio file!: %s / Audio must be initialized!\n",
                   filename);
}

void audio_narrator_play(const char *filename)
{
    if (audio_state && !audio_paused && config_get_d(CONFIG_NARRATOR_VOLUME))
    {
        while (lock_hold) {}
        struct voice *V;

        /* If we're already playing this sound, preempt the running copy. */

        lock_hold = 1;
        {
            for (V = narrators; V; V = V->next)
                if (strcmp(V->name, filename) == 0)
                {
                    ov_raw_seek(&V->vf, 0);

                    V->amp = 1.0f;

                    if (V->amp > 1.0f) V->amp = 1.0;
                    if (V->amp < 0.0f) V->amp = 0.0;

                    lock_hold = 0;
                    return;
                }
        }
        lock_hold = 0;

        /* Create a new voice structure. */

        V = voice_init(filename, 1.0f);

        /* Add it to the list of sounding voices. */

        lock_hold = 1;
        {
            V->next = narrators;
            narrators = V;
        }
        lock_hold = 0;
    }
    else if (!audio_state && !audio_paused)
        log_errorf("Failure to open audio file!: %s / Audio must be initialized!\n",
                   filename);
}

/*---------------------------------------------------------------------------*/

void audio_music_play(const char *filename)
{
    if (audio_state && !audio_paused)
    {
        while (lock_hold) {}
        audio_music_stop();

        lock_hold = 1;
        {
            if ((music = voice_init(filename, 0.0f)))
                music->loop = 1;
        }
        lock_hold = 0;
    }
    else if (!audio_state && !audio_paused)
        log_errorf("Failure to open audio file!: %s / Audio must be initialized!\n",
                   filename);
}

void audio_music_queue(const char *filename, float t)
{
    float clampedTime = CLAMP(0.001f, t, 1.0f);

    if (audio_state)
    {
        while (lock_hold) {}
        lock_hold = 1;
        {
            if ((queue = voice_init(filename, 0.0f)))
            {
                queue->loop = 1;

                if (clampedTime > 0.0f)
                    queue->damp = +1.0f / (AUDIO_RATE * clampedTime);
            }
        }
        lock_hold = 0;
    }
    else if (!audio_state && !audio_paused)
        log_errorf("Failure to open audio file!: %s / Audio must be initialized!\n",
                   filename);
}

void audio_music_stop(void)
{
    if (audio_state)
    {
        while (lock_hold) {}
        lock_hold = 1;
        {
            if (music)
                voice_free(music);

            music = NULL;
        }
        lock_hold = 0;
    }
}

/*---------------------------------------------------------------------------*/

void audio_music_fade_out(float t)
{
    float clampedTime = CLAMP(0.001f, t, 1.0f);

    while (lock_hold) {}
    lock_hold = 1;
    {
        if (music) music->damp = -1.0f / (AUDIO_RATE * clampedTime);
    }
    lock_hold = 0;
}

void audio_music_fade_in(float t)
{
    float clampedTime = CLAMP(0.001f, t, 1.0f);

    while (lock_hold) {}
    lock_hold = 1;
    {
        if (music) music->damp = +1.0f / (AUDIO_RATE * clampedTime);
    }
    lock_hold = 0;
}

void audio_music_fade_to(float t, const char *filename)
{
    float clampedTime = CLAMP(0.001f, t, 1.0f);

    if (music)
    {
        if (!music->name)
        {
            audio_music_stop();
            audio_music_play(filename);
            audio_music_fade_in(clampedTime);
            return;
        }

        if (strcmp(filename, music->name) == 0)
        {
            /*
             * We're fading to the current track.  Chances are,
             * whatever track is still in the queue, we don't want to
             * hear it anymore.
             */

            if (queue)
            {
                voice_free(queue);
                queue = NULL;
            }

            audio_music_fade_in(clampedTime);
        }
        else
        {
            audio_music_fade_out(t);
            audio_music_queue(filename, clampedTime);
        }
    }
    else
    {
        audio_music_play(filename);
        audio_music_fade_in(clampedTime);
    }
}

/*---------------------------------------------------------------------------*/

/*
 * Logarithmic volume control.
 */
void audio_volume(int master, int snd, int mus, int narrator)
{
    float master_logaritmic   = (float)  master   / 10.0f;
    float sound_logaritmic    = (float) (snd      / 10.0f) * master_logaritmic;
    float music_logaritmic    = (float) (mus      / 10.0f) * master_logaritmic;
    float narrator_logaritmic = (float) (narrator / 10.0f) * master_logaritmic;

    master_vol   = LOGF_VOLUME(master_logaritmic);
    sound_vol    = LOGF_VOLUME(sound_logaritmic);
    music_vol    = LOGF_VOLUME(music_logaritmic);
    narrator_vol = LOGF_VOLUME(narrator_logaritmic);
}

/*---------------------------------------------------------------------------*/
