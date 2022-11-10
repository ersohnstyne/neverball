#ifndef AUDIO_H
#define AUDIO_H

/*---------------------------------------------------------------------------*/

int audio_available(void);

void audio_tick(void);
void audio_setspeed(float);
void audio_init(void);
void audio_free(void);
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
