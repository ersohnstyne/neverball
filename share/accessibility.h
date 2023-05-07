#ifndef ACCESSIBILITY_H
#define ACCESSIBILITY_H

#include "base_config.h"

extern int accessibility_busy;

extern int ACCESSIBILITY_SLOWDOWN;

void accessibility_init(void);
void accessibility_load(void);
void accessibility_save(void);

void accessibility_set_d(int, int);
void accessibility_tgl_d(int);
int  accessibility_tst_d(int, int);
int  accessibility_get_d(int);

#endif