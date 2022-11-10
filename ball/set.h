#ifndef SET_H
#define SET_H

#include "base_config.h"
#include "level.h"
#ifndef FS_VERSION_1
#include "package.h"
#endif

#define SET_FILE "sets.txt"
#define SET_MISC "set-misc.txt"

#define BOOST_FILE "boost-rushes.txt"

#define MAXLVL_SET 50

#define SET_UNLOCKABLE_EDITION 2

/*---------------------------------------------------------------------------*/

int  set_init(int);
void set_quit(void);

/*---------------------------------------------------------------------------*/

int  set_exists(int);
void set_goto(int);
int  set_find(const char *);

int  curr_set(void);

#ifndef FS_VERSION_1
int  set_is_downloadable(int);
int  set_is_downloading(int);
int  set_is_installed(int);
int  set_download(int, struct fetch_callback);
#endif

const char         *set_id(int);
const char         *set_name(int);
const char         *set_name_n(int);
const char         *set_file(int);
const char         *set_desc(int);
const char         *set_shot(int);
const struct score *set_score(int, int);

int  set_score_update (int, int, int *, int *);
void set_rename_player(int, int, const char *);

void set_store_hs(void);

/*---------------------------------------------------------------------------*/

struct level *get_level(int);

void level_snap(int, const char *);
void set_cheat(void);
void set_detect_bonus_product(void);

/*---------------------------------------------------------------------------*/

#endif
