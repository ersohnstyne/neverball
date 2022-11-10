#ifndef ST_CONF_H
#define ST_CONF_H

#include "state.h"

extern int conf_covid_extended;
extern int online_mode;

extern struct state st_conf_covid_extend;
extern struct state st_conf_account;
extern struct state st_conf_social;
extern struct state st_conf_notification;
extern struct state st_conf_control;
extern struct state st_conf_calibrate;
extern struct state st_conf_controllers;
extern struct state st_conf_audio;
extern struct state st_conf;
extern struct state st_null;

extern struct state st_conf_video;
extern struct state st_conf_display;

int goto_conf(struct state *back_state, int using_game);
int goto_conf_covid_extend(struct state *returnable, int auto_extend, int draw_back);
void conf_covid_retract(void);

#endif
