#ifndef ST_TRANSFER_H
#define ST_TRANSFER_H

#include "state.h"

void transfer_add_dispatch_event(void (*request_addreplay_dispatch_event)(int status_limit));
void transfer_addreplay(const char* path);
void transfer_addreplay_exceeded();

extern struct state st_transfer;

#endif
