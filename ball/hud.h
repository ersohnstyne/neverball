#ifndef HUD_H
#define HUD_H

/*---------------------------------------------------------------------------*/

void toggle_hud_visibility(int);
int  hud_visibility(void);

void hud_init(void);
void hud_free(void);

void hud_set_alpha(float);
void hud_paint(void);
void hud_timer(float);
void hud_update(int, float);
void hud_update_camera_direction(float);

void hud_speedup_reset(void);
void hud_speedup_pulse(void);
void hud_speedup_timer(float);
void hud_speedup_paint(void);

void hud_cam_pulse(int);
void hud_cam_timer(float);
void hud_cam_paint(void);

void hud_speed_pulse(int);
void hud_speed_timer(float);
void hud_speed_paint(void);

/*---------------------------------------------------------------------------*/

#endif
