#ifndef MAIN_SHARE_H
#define MAIN_SHARE_H

/**
 * Used for linux or win32 cli main entry function point
 */
int main_share(int, char **);

#ifdef __EMSCRIPTEN__

//void WGCL_CallMain(const char *);
void WGCL_CallMain(const int);

#endif

#endif
