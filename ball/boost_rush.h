#ifndef BOOST_RUSH_H
#define BOOST_RUSH_H

void boost_rush_init(void);
void boost_rush_stop(void);
void collect_coin_value(int);
void increase_speed(void);
void substract_speed(void);
void boost_rush_timer(float);

float get_speed_indicator(void);
int is_sonic(void);

#endif
