#ifndef MAIN_SHARE_H
#define MAIN_SHARE_H

int main_share(int, char **);

#ifdef __EMSCRIPTEN__

void WGCLCallMain_Lite(void);
void WGCLCallMain_Home(void);
void WGCLCallMain_Pro(void);
void WGCLCallMain_Enterprise(void);
void WGCLCallMain_Education(void);

void WGCLCallMain_Srv_Essentials(void);
void WGCLCallMain_Srv_Standard(void);
void WGCLCallMain_Srv_Datacenter(void);

#endif

#endif
