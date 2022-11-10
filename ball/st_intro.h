#ifndef ST_INTRO_H
#define ST_INTRO_H

#define SKIP_END_SUPPORT

#include "state.h"

extern struct state st_intro;
extern struct state st_intro_accn_disabled;
extern struct state st_intro_restore;
extern struct state st_intro_nointernet;
extern struct state st_intro_waitinternet;

extern struct state st_screensaver;
extern struct state st_server_maintenance;

#endif
