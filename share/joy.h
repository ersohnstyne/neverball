#ifndef JOY_H
#define JOY_H

int  joy_init(void);
void joy_quit(void);

void joy_add(int device);
void joy_remove(int instance);
int  joy_button(int instance, int b, int d);
void joy_axis(int instance, int a, float v);

void joy_active_cursor(int instance, int active);
int  joy_get_active_cursor(int instance);

int  joy_get_gamepad_action_index(void);
int  joy_get_cursor_actions(int instance);

int  joy_connected(int instance, int *battery_level, int *wired);

#if NB_PB_WITH_XBOX==1
void joy_update(void);
#endif

#endif