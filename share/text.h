#ifndef TEXT_H
#define TEXT_H

#if _WIN32 && __GNUC__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif

#ifndef MAXSTR
#define MAXSTR MAX_STR_BLOCKREASON
#endif

/*---------------------------------------------------------------------------*/

int text_add_char(Uint32, char *, int);
int text_del_char(char *);
int text_length(const char *);

/*---------------------------------------------------------------------------*/

extern char text_input[MAXSTR];

void text_input_start(void (*cb)(int typing));
void text_input_stop(void);
int  text_input_str(const char *, int typing);
int  text_input_char(int);
int  text_input_del(void);

#endif
