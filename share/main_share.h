#ifndef MAIN_SHARE_H
#define MAIN_SHARE_H

int main_share(int, char **);

#ifdef __EMSCRIPTEN__

int WGCLCallMain_Lite(void);
int WGCLCallMain_Home(void);
int WGCLCallMain_Pro(void);
int WGCLCallMain_Enterprise(void);
int WGCLCallMain_Education(void);

int WGCLCallMain_Srv_Essentials(void);
int WGCLCallMain_Srv_Standard(void);
int WGCLCallMain_Srv_Datacenter(void);

#endif

#endif
