#ifndef ST_SAVE_H
#define ST_SAVE_H

#include "state.h"

extern struct state st_save;
extern struct state st_clobber;
extern struct state st_lockdown;
extern struct state st_save_error;

int goto_save(struct state *, struct state *);

#endif
