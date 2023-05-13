#ifndef ST_CONF_H
#define ST_CONF_H

#include "state.h"

extern int conf_covid_extended;
extern int online_mode;

extern struct state st_conf_account;
extern struct state st_conf;
extern struct state st_null;

extern struct state st_conf_video;
extern struct state st_conf_display;

int goto_conf(struct state *, int, int);
void conf_covid_retract(void);

#endif
