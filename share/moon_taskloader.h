#ifndef MOON_TASKLOADER_H
#define MOON_TASKLOADER_H

struct moon_taskloader_execute
{
    char *filename;
    int   time_ms;
    int   goal_enabled;
};

struct moon_taskloader_progress
{
    double total;
    double now;
};

struct moon_taskloader_done
{
    unsigned int finished : 1;
};

struct moon_taskloader_callback
{
    int  (*execute)  (void *data, void *execute_data);
    void (*progress) (void *data, void *progress_data);
    void (*done)     (void *data, void *done_data);
    void *data;
};

struct game_moon_taskloader_info
{
    struct moon_taskloader_callback callback;
    char                           *filename;
};

void moon_taskloader_init(void (*dispatch_event) (void *));
void moon_taskloader_handle_event(void *);
void moon_taskloader_quit(void);

unsigned int moon_taskloader_load(const char *,
                                  struct moon_taskloader_callback);

#endif
