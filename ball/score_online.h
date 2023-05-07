#ifndef SCORE_ONLINE_H
#define SCORE_ONLINE_H

#if NB_STEAM_API==1
void score_steam_init_hs(int, int);
int score_steam_hs_loading(void);
int score_steam_hs_online(void);

void score_steam_hs_save(int, int);
#endif

#endif