#ifndef ST_SHOP_H
#define ST_SHOP_H

#include "state.h"

extern struct state st_shop;
extern struct state st_shop_unlocked;
extern struct state st_shop_rename;
extern struct state st_shop_unregistered;
extern struct state st_shop_iap;
extern struct state st_shop_buy;

int goto_shop_rename(struct state *, struct state *, unsigned int);
int goto_shop_iap(struct state *, struct state *, int, int, int);

#endif
