#ifndef ST_TRANSFER_H
#define ST_TRANSFER_H

#include "state.h"

#define ENABLE_GAME_TRANSFER 0
#define GAME_TRANSFER_TARGET 1

#if GAME_TRANSFER_TARGET==0
void transfer_add_dispatch_event(void (*request_addreplay_dispatch_event)(int status_limit));
void transfer_addreplay(const char *path);
void transfer_addreplay_exceeded();
#endif

#if ENABLE_GAME_TRANSFER==1
extern struct state st_transfer;
#endif

#endif