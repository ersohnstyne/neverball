#ifndef ST_MALFUNCTION_H
#define ST_MALFUNCTION_H

/*
 * If there is more than 10 pressed without released keys,
 * an malfunction popup dialog will appear.
 */
#define MALFUNCTION_THRESHOLD 10

extern int malfunction_locked;
extern int char_downcounter[128];
extern int arrow_downcounter[5];
extern int fwindow_downcounter[13];

/*
 * HACK: Does not makes any sense
 *
 * extern struct state st_malfunction;
 * extern struct state st_handsoff;
 */

int check_malfunctions(void);
int check_handsoff(void);

int goto_handsoff(struct state *back);

#endif