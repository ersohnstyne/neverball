#ifndef MAIN_SHARE_H
#define MAIN_SHARE_H

int main_share(int, char **);

#ifdef __EMSCRIPTEN__

//void WGCL_CallMain(const char *);
void WGCL_CallMain(const int);

#endif

#endif
