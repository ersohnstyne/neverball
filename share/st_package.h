#ifndef ST_PACKAGE
#define ST_PACKAGE 1

#include "state.h"

/* === PREFIX/SUFFIX CONVERTER === */

/* This suffix variable name "*_package" will be replaced into "*_addons". */
#define st_package st_addons

/* This suffix function name "*_package" will be replaced into "*_addons". */
#define goto_package goto_addons

/* This prefix function name "package_*" will be replaced into "addons_*". */
#define package_set_installed_action addons_set_installed_action

/* === END PREFIX/SUFFIX CONVERTER === */

extern struct state st_package;

void goto_package(int, struct state *);
void package_set_installed_action(int (*installed_action_fn)(int pi));

#endif