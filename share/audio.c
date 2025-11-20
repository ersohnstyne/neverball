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

#ifndef __EMSCRIPTEN__

#if _WIN32 && __MINGW32__
#include <SDL2/SDL.h>
#elif _WIN32 && _MSC_VER
#include <SDL.h>
#elif _WIN32
#error Security compilation error: No target include file in path for Windows specified!
#else
#include <SDL.h>
#endif

#define OV_EXCLUDE_STATIC_CALLBACKS
#if defined(__WII__)
#include <tremor/ivorbiscodec.h>
#include <tremor/ivorbisfile.h>
#else
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <math.h>

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
#pragma message(__FILE__": Missing _CRT_MAP_ALLOC, recreate: _CRTDBG_MAP_ALLOC + crtdbg.h")
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#endif

#ifdef _MSC_VER
#pragma comment(lib, "libogg.lib")
#pragma comment(lib, "libvorbis_static.lib")
#pragma comment(lib, "libvorbisfile_static.lib")
#endif

/*---------------------------------------------------------------------------*/

#if defined(__WII__)
#define AUDIO_RATE 32000
#else
#define AUDIO_RATE 44100
#endif
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

#if defined(__WII__)
#define VOICE_CACHE_COUNT 8 /* Was: 16 */

#define SDL_OpenAudioDevice(_name, _capture, _req, _res, _changes) \
        SDL_OpenAudio(_req, _res)

#define SDL_PauseAudioDevice(_devid, _paused) \
        SDL_PauseAudio(_paused)

#define SDL_CloseAudioDevice(_devid) \
        SDL_CloseAudio()

static struct voice *voice_cache[VOICE_CACHE_COUNT];
#endif

static struct voice *voices_music     = NULL;
static struct voice *voices_queue     = NULL;
static struct voice *voices_sfx       = NULL;
static struct voice *voices_narrators = NULL;
static short        *buffer           = NULL;

static ov_callbacks callbacks = {
    fs_ov_read, fs_ov_seek, fs_ov_close, fs_ov_tell
};

/*---------------------------------------------------------------------------*/

#define SOUND_VOICE_COUNT 2

#define LOGF_VOLUME(v) ((float) powf((v), 2.0f))
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
#if defined(__WII__)
        if ((n = (int) ov_read(&V->vf, ibuf, r, &b)) > 0)
#else
        if ((n = (int) ov_read(&V->vf, ibuf, r, order, 2, 1, &b)) > 0)
#endif
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

#if defined(__WII__)
void voice_free(struct voice *V);
#endif

static struct voice *voice_init(const char *filename, float a)
{
    struct voice *VP = NULL;
    fs_file       fp;

#if defined(__WII__)
    int i;

    /* Search cache for voice */

    for (i = 0; i < VOICE_CACHE_COUNT; i++)
    {
        if (voice_cache[i] != NULL)
        {
            if (strcmp(filename, voice_cache[i]->name) == 0)
            {
                VP = voice_cache[i];
                VP->amp  = a;
                VP->damp = 0;
                VP->play = 1;
                VP->loop = 0;
                ov_raw_seek(&VP->vf, 0);
                return VP;
            }
        }
    }
#endif

    /* Allocate and initialize a new voice structure. */

    if ((VP = (struct voice *) calloc(1, sizeof (struct voice))))
    {
        /* Note the name. */

        VP->name = strdup(filename);

        /* Attempt to open the named Ogg stream. */

        if ((fp = fs_open_read(filename)))
        {
            if (ov_open_callbacks(fp, &VP->vf, NULL, 0, callbacks) == 0)
            {
                vorbis_info *info = ov_info(&VP->vf, -1);

                /* On success, configure the voice. */

                VP->amp  = a;
                VP->damp = 0;
                VP->chan = info->channels;
                VP->play = 1;
                VP->loop = 0;

                if (VP->amp > 1.0f) VP->amp = 1.0;
                if (VP->amp < 0.0f) VP->amp = 0.0;

                /* The file will be closed when the Ogg is cleared. */
            }
            else fs_close(fp);
        }
    }

#if defined(__WII__)
    /* Put it in the cache */

    for (i = 0; i < VOICE_CACHE_COUNT; i++)
    {
        if (voice_cache[i] == NULL)
        {
            voice_cache[i] = VP;
            return VP;
        }
    }

    /* Replace a voice that isn't playing */

    for (i = 0; i < VOICE_CACHE_COUNT; i++)
    {
        if (voice_cache[i] != NULL && !voice_cache[i]->play)
        {
            voice_free(voice_cache[i]);
            voice_cache[i] = VP;
            return VP;
        }
    }
#endif

    return VP;
}

void voice_free(struct voice *V)
{
    if (V)
    {
        ov_clear(&V->vf);

        free(V->name); V->name = NULL;
        free(V); V = NULL;
    }
}

#if !defined(__WII__)
static void voice_quit(struct voice* V)
{
    struct voice *VP;

    for (VP = V; VP; )
    {
        VN = VP->next;
        voice_free(VP); VP = VN;
    }
}
#endif

/*---------------------------------------------------------------------------*/

static void audio_step(void *data, Uint8 *stream, int length)
{
    while (lock_hold) {}
    lock_hold = 1;

    struct voice *Vs[SOUND_VOICE_COUNT] = {
        voices_sfx,
        voices_narrators
    };

    struct voice *VPs[SOUND_VOICE_COUNT] = {
        NULL,
        NULL
    };

    float voices_vols[SOUND_VOICE_COUNT] = {
        sound_vol,
        narrator_vol
    };

    /* Zero the output buffer. */

    memset(stream, 0, length);

    /* Mix the background music. */

    if (voices_music)
    {
        if (voices_music->play &&
            voice_step(voices_music, music_vol, stream, length))
        {
#if defined(__WII__)
            voices_music->play = 0;
#else
            voice_free(voices_music);
#endif

            if (voices_queue)
            {
                voices_music = voices_queue;
                voices_queue = NULL;
            }
            else voices_music = NULL;
        }

        /* If the track has faded out, move to a queued track. */

        if (voices_music && voices_music->amp <= 0.0f && voices_music->damp < 0.0f && voices_queue)
        {
#if defined(__WII__)
            voices_music->play = 0;
#else
            voice_free(voices_music);
#endif

            if (voices_queue)
            {
                voices_music = voices_queue;
                voices_queue = NULL;
            }
        }
    }

    /* Iterate over all active sound voices. */

    for (int i = 0; i < SOUND_VOICE_COUNT; i++)
        while (Vs[i])
        {
            /* Mix this sound voice. */

            if (Vs[i]->play && voice_step(Vs[i], voices_vols[i], stream, length))
            {
                /* Delete a finished voice... */

                struct voice *VT = Vs[i];

                if (VPs[i])
                    Vs[i] = VPs[i]->next = Vs[i]->next;
                else switch (i)
                {
                    case 0: Vs[i] = voices_sfx       = Vs[i]->next; break;
                    case 1: Vs[i] = voices_narrators = Vs[i]->next; break;
                }

#if defined(__WII__)
                VT->play = 0;
#else
                voice_free(VT);
#endif
                VT = NULL;
            }
            else
            {
                /* ... or continue to the next. */

                VPs[i] = Vs[i];
                Vs[i]  = Vs[i]->next;
            }
        }

    lock_hold = 0;
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

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
    {
        log_printf("Failure to initialize audio (%s)\n", SDL_GetError());
        return;
    }

    audio_state = 0;

    /* Configure the audio. */

    memset(&device_spec, 0, sizeof (device_spec));
    device_spec.format   = AUDIO_S16;
    device_spec.channels = AUDIO_CHAN;
    device_spec.samples  = config_get_d(CONFIG_AUDIO_BUFF);
    device_spec.freq     = AUDIO_RATE * (float) (accessibility_get_d(ACCESSIBILITY_SLOWDOWN) / 100.0f);
    device_spec.callback = audio_step;

    memset(&spec, 0, sizeof (spec));
    spec.format   = AUDIO_S16;
    spec.channels = AUDIO_CHAN;
    spec.samples  = config_get_d(CONFIG_AUDIO_BUFF);
    spec.freq     = AUDIO_RATE * (float) (accessibility_get_d(ACCESSIBILITY_SLOWDOWN) / 100.0f);
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

    audio_volume(CLAMP(0, config_get_d(CONFIG_MASTER_VOLUME),   10),
                 CLAMP(0, config_get_d(CONFIG_SOUND_VOLUME),    10),
                 CLAMP(0, config_get_d(CONFIG_MUSIC_VOLUME),    10),
                 CLAMP(0, config_get_d(CONFIG_NARRATOR_VOLUME), 10));
}

void audio_free(void)
{
    if (!audio_state) return;

    /* Halt the audio thread. */

    if (audio_state)
        SDL_CloseAudioDevice(audio_device_id);

    audio_device_id = -1;

    /* Release the input buffer. */

    if (buffer)
    {
        free(buffer);
        buffer = NULL;
    }

    /* Free the voices. */

#if !defined(__WII__)
    if (voices_queue)
    {
        voice_quit(voices_queue);
        voices_queue = NULL;
    }

    if (voices_music)
    {
        voice_quit(voices_music);
        voices_music = NULL;
    }

    if (voices_sfx)
    {
        voice_quit(voices_sfx);
        voices_sfx = NULL;
    }

    if (voices_narrators)
    {
        voice_quit(voices_narrators);
        voices_narrators = NULL;
    }

    for (V = voices_music; V;)
    {
        struct voice *N = V ? V->next : NULL;
        voice_free(V);
        V = N;
    }

    voices_music = NULL;

    for (V = voices_sfx; V;)
    {
        struct voice *N = V ? V->next : NULL;
        voice_free(V);
        V = N;
    }

    voices_sfx = NULL;

    for (V = voices_narrators; V;)
    {
        struct voice *N = V ? V->next : NULL;
        voice_free(V);
        V = N;
    }

    voices_narrators = NULL;

#endif

    SDL_QuitSubSystem(SDL_INIT_AUDIO);

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
    /* Can't play game sound, master volume was set to 0. */

    if (config_get_d(CONFIG_MASTER_VOLUME) == 0) return;

    if (audio_state && !audio_paused && config_get_d(CONFIG_SOUND_VOLUME))
    {
        if (!filename || !filename[0])
        {
            log_errorf("filename returned 0!\n");
            return;
        }

        while (lock_hold) {}
        struct voice *VSFX;

        /* If we're already playing this sound, preempt the running copy. */

        lock_hold = 1;
        {
            for (VSFX = voices_sfx; VSFX; VSFX = VSFX->next)
                if (strcmp(VSFX->name, filename) == 0)
                {
                    ov_raw_seek(&VSFX->vf, 0);

                    VSFX->amp = a;

                    if (VSFX->amp > 1.0f) VSFX->amp = 1.0;
                    if (VSFX->amp < 0.0f) VSFX->amp = 0.0;

                    lock_hold = 0;
                    return;
                }
        }
        lock_hold = 0;

        /* Create a new voice structure. */

        VSFX = voice_init(filename, a);

        /* Add it to the list of sounding voices. */

        lock_hold = 1;
        {
            VSFX->next = voices_sfx;
            voices_sfx = VSFX;
        }
        lock_hold = 0;
    }
    else if (!audio_state && !audio_paused)
    {
        log_errorf("Failure to open audio file!: %s / Audio must be initialized!\n",
                   filename);
#if _DEBUG
        SDL_TriggerBreakpoint();
#endif
    }
}

void audio_narrator_play(const char *filename)
{
    /* Can't play narrator sound, master volume was set to 0. */

    if (config_get_d(CONFIG_MASTER_VOLUME) == 0) return;

    if (audio_state && !audio_paused && config_get_d(CONFIG_NARRATOR_VOLUME))
    {
        if (!filename || !filename[0])
        {
            log_errorf("filename returned 0!\n");
            return;
        }

        while (lock_hold) {}
        struct voice *VNFX;

        /* If we're already playing this sound, preempt the running copy. */

        lock_hold = 1;
        {
            for (VNFX = voices_narrators; VNFX; VNFX = VNFX->next)
                if (strcmp(VNFX->name, filename) == 0)
                {
                    ov_raw_seek(&VNFX->vf, 0);

                    VNFX->amp = 1.0f;

                    if (VNFX->amp > 1.0f) VNFX->amp = 1.0;
                    if (VNFX->amp < 0.0f) VNFX->amp = 0.0;

                    lock_hold = 0;
                    return;
                }
        }
        lock_hold = 0;

        /* Create a new voice structure. */

        VNFX = voice_init(filename, 1.0f);

        /* Add it to the list of sounding voices. */

        lock_hold = 1;
        {
            VNFX->next = voices_narrators;
            voices_narrators = VNFX;
        }
        lock_hold = 0;
    }
    else if (!audio_state && !audio_paused)
    {
        log_errorf("Failure to open audio file!: %s / Audio must be initialized!\n",
                   filename);
#if _DEBUG
        SDL_TriggerBreakpoint();
#endif
    }
}

/*---------------------------------------------------------------------------*/

static void audio_music_play(const char *filename, int loop)
{
    if (audio_state && !audio_paused)
    {
        if (!filename || !filename[0])
        {
            log_errorf("filename returned 0!\n");
            return;
        }

        while (lock_hold) {}
        audio_music_stop();

        lock_hold = 1;
        {
            if ((voices_music = voice_init(filename, 0.0f)))
                voices_music->loop = loop;
        }
        lock_hold = 0;
    }
    else if (!audio_state && !audio_paused)
    {
        log_errorf("Failure to open audio file!: %s / Audio must be initialized!\n",
                   filename);
#if _DEBUG
        SDL_TriggerBreakpoint();
#endif
    }
}

static void audio_music_queue(const char *filename, float t, int loop)
{
    float clampedTime = CLAMP(0.001f, t, 1.0f);

    if (audio_state)
    {
        while (lock_hold) {}
        lock_hold = 1;
        {
            if ((voices_queue = voice_init(filename, 0.0f)))
            {
                voices_queue->loop = loop;

                if (clampedTime > 0.0f)
                    voices_queue->damp = +1.0f / (AUDIO_RATE * clampedTime);
            }
        }
        lock_hold = 0;
    }
    else if (!audio_state && !audio_paused)
    {
        log_errorf("Failure to open audio file!: %s / Audio must be initialized!\n",
                   filename);
#if _DEBUG
        SDL_TriggerBreakpoint();
#endif
    }
}

void audio_music_stop(void)
{
    if (audio_state)
    {
#if defined(__WII__)
        if (voices_music)
        {
            voices_music->play = 0;
            voices_music = NULL;
        }
#else
        while (lock_hold) {}
        lock_hold = 1;
        if (voices_music)
        {
            voice_free(voices_music);
            voices_music = NULL;
        }
        lock_hold = 0;
#endif
    }
}

/*---------------------------------------------------------------------------*/

void audio_music_fade_out(float t)
{
    float clamped_time = CLAMP(0.001f, t, 1.0f);

    while (lock_hold) {}
    lock_hold = 1;
    {
        if (voices_music) voices_music->damp = -1.0f / (AUDIO_RATE * clamped_time);
    }
    lock_hold = 0;
}

void audio_music_fade_in(float t)
{
    float clamped_time = CLAMP(0.001f, t, 1.0f);

    while (lock_hold) {}
    lock_hold = 1;
    {
        if (voices_music) voices_music->damp = +1.0f / (AUDIO_RATE * clamped_time);
    }
    lock_hold = 0;
}

void audio_music_fade_to(float t, const char *filename, int loop)
{
    float clamped_time = CLAMP(0.001f, t, 1.0f);

    if (voices_music)
    {
        if (!voices_music->name)
        {
            audio_music_stop();
            audio_music_play(filename, loop);
            audio_music_fade_in(clamped_time);
            return;
        }

        if (strcmp(filename, voices_music->name) == 0)
        {
            /*
             * We're fading to the current track.  Chances are,
             * whatever track is still in the queue, we don't want to
             * hear it anymore.
             */

            if (voices_queue)
            {
#if defined(__WII__)
                voices_queue->play = 0;
#else
                voice_free(voices_queue);
#endif
                voices_queue = NULL;
            }

            audio_music_fade_in(clamped_time);
        }
        else
        {
            audio_music_fade_out(clamped_time);
            audio_music_queue(filename, clamped_time, loop);
        }
    }
    else
    {
        /* Track is not playing, just start immediately. */

        audio_music_play(filename, loop);
        audio_music_fade_in(clamped_time);
    }
}

/*---------------------------------------------------------------------------*/

/*
 * Logarithmic volume control (must have calculating stuffs perks).
 */
void audio_volume(int master, int sound, int music, int narrator)
{
    float master_logaritmic   = (float)  master   / 10.0f;
    float sound_logaritmic    = (float) (sound    / 10.0f) * master_logaritmic;
    float music_logaritmic    = (float) (music    / 10.0f) * master_logaritmic;
    float narrator_logaritmic = (float) (narrator / 10.0f) * master_logaritmic;

    master_vol   = LOGF_VOLUME(master_logaritmic);
    sound_vol    = LOGF_VOLUME(sound_logaritmic);
    music_vol    = LOGF_VOLUME(music_logaritmic);
    narrator_vol = LOGF_VOLUME(narrator_logaritmic);
}

/*---------------------------------------------------------------------------*/

#endif
