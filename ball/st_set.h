#ifndef ST_SET_H
#define ST_SET_H

#include "state.h"

void set_boost_on(int);

extern int is_boost_on(void);

extern struct state st_set;
extern struct state st_campaign;

int goto_playgame(void);
int goto_playgame_register(void);
int goto_playmenu(int);

#endif
