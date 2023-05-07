#ifndef POWERUP_H
#define POWERUP_H

#define ENABLE_POWERUP

extern void powerup_stop(void);

extern void init_earninator(void);
extern void init_floatifier(void);
extern void init_speedifier(void);

/*---------------------------------------------------------------------------*/

extern float get_coin_multiply(void);
extern float get_gravity_multiply(void);
extern float get_tilt_multiply(void);

#endif
