#ifndef ST_NAME_H
#define ST_NAME_H

#include "state.h"

extern struct state st_name;

int goto_name(struct state *ok, struct state *cancel,
	          int (*new_ok_fn)(struct state *), int (*new_cancel_fn)(struct state *),
	          unsigned int back);

#endif
