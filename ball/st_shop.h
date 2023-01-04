#ifndef ST_SHOP_H
#define ST_SHOP_H

#include "state.h"

extern struct state st_shop;
extern struct state st_shop_unlocked;

int goto_shop_rename(struct state *, struct state *, unsigned int);
int goto_shop_iap(struct state *, struct state *,
                  int (*new_ok_fn)(struct state *), int (*new_cancel_fn)(struct state *),
                  int, int, int);

int goto_shop_activate(struct state* ok, struct state* cancel,
                       int (*new_ok_fn)(struct state*), int (*new_cancel_fn)(struct state*));

#endif
