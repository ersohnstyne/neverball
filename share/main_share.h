#ifndef MAIN_SHARE_H
#define MAIN_SHARE_H

int main_share(int, char **);

#ifdef __EMSCRIPTEN__

void WGCLCallMain(int);

#endif

#endif
