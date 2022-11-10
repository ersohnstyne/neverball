#ifndef ST_SET_H
#define ST_SET_H

#include "state.h"

void set_boost_on(int);

extern int is_boost_on(void);

extern struct state st_set;
extern struct state st_campaign;

#ifdef LEVELGROUPS_INCLUDES_CAMPAIGN
extern struct state st_levelgroup;
#endif

#endif
