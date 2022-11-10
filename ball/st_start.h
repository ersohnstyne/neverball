#ifndef ST_START_H
#define ST_START_H

#include "state.h"

extern struct state st_start;
#if NB_HAVE_PB_BOTH == 1
extern struct state st_start_unavailable;
#endif
extern struct state st_start_compat;

#endif
