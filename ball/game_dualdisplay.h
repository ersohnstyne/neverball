#ifndef GAME_DUALDISPLAY_H
#define GAME_DUALDISPLAY_H

#if ENABLE_DUALDISPLAY==1
int  game_dualdisplay_init(void);
void game_dualdisplay_quit(void);

void game_dualdisplay_gui_init(void);
void game_dualdisplay_gui_free(void);

void game_dualdisplay_set_mode(int);
void game_dualdisplay_set_time(int);
void game_dualdisplay_set_heart(int);

void game_dualdisplay_draw(void);
#endif

#endif