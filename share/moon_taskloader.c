/*
 * Copyright (C) 2024 Microsoft / Neverball authors
 *
 * NEVERBALL is  free software; you can redistribute  it and/or modify
 * it under the  terms of the GNU General  Public License as published
 * by the Free  Software Foundation; either version 2  of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 */

#include "moon_taskloader.h"
#include "log.h"
#include "fs.h"

#include "solid_base.h"

#if _WIN32 && __MINGW32__
#include <SDL2/SDL_mutex.h>
#include <SDL2/SDL_thread.h>
#else
#include <SDL_mutex.h>
#include <SDL_thread.h>
#endif

#if _DEBUG && _MSC_VER
#ifndef _CRTDBG_MAP_ALLOC
#pragma message(__FILE__": Missing CRT-Debugger include header, recreate: crtdbg.h")
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#endif

/*
 * Moon taskloader has parallel thread-safe to keep rendering the game
 * during loading the map. May be asynchronous.
 */

/*---------------------------------------------------------------------------*/

static unsigned int last_moon_taskloader_id = 0;

struct moon_taskloader_info
{
    struct moon_taskloader_callback callback;

    char          *filename;
    void          *execute_data;
    unsigned int   moon_taskloader_id;
};

static List moon_taskloader_list = NULL;

/*---------------------------------------------------------------------------*/

struct moon_taskloader_event
{
    void (*callback) (void *, void *);
    void *callback_data;
    void *extra_data;
};

/*
 * Dispatch a wrapped callback to the thread that calls
 * moon_taskloader_handle_event.
 */
static void (*moon_taskloader_dispatch_event) (void *) = NULL;

/*
 * Create extra_data for a execute callback.
 */
static struct moon_taskloader_execute *create_extra_execute(const char *filename, int time_ms, int goal_enabled)
{
    struct moon_taskloader_execute *exec = (struct moon_taskloader_execute *) calloc(sizeof (*exec), 1);

    if (exec)
    {
        exec->filename     = filename;
        exec->time_ms      = time_ms;
        exec->goal_enabled = goal_enabled;
    }

    return exec;
}

/*
 * Create extra_data for a progress callback.
 */
static struct moon_taskloader_progress *create_extra_progress(double total, double now)
{
    struct moon_taskloader_progress *pr = (struct moon_taskloader_progress *) calloc(sizeof (*pr), 1);

    if (pr)
    {
        pr->total = total;
        pr->now   = now;
    }

    return pr;
}

/*
 * Create extra_data for a done callback.
 */
static struct moon_taskloader_done *create_extra_done(int finished)
{
    struct moon_taskloader_done *dn = (struct moon_taskloader_done *) calloc(sizeof (*dn), 1);

    if (dn)
        dn->finished = !!finished;

    return dn;
}

/*
 * Free previously created extra_data.
 */
static void free_extra_data(void *extra_data)
{
    if (extra_data)
    {
        free(extra_data);
        extra_data = NULL;
    }
}

/*
 * Allocate a callback wrapper structure.
 */
static struct moon_taskloader_event *create_moon_taskloader_event(void)
{
    struct moon_taskloader_event *mtle = (struct moon_taskloader_event *) calloc(sizeof (*mtle), 1);

    return mtle;
}

static void free_moon_taskloader_event(struct moon_taskloader_event *mtle)
{
    if (mtle)
    {
        if (mtle->callback_data)
        {
            free_extra_data(mtle->callback_data);
            mtle->callback_data = NULL;
        }

        if (mtle->extra_data)
        {
            free_extra_data(mtle->extra_data);
            mtle->extra_data = NULL;
        }

        free(mtle);
        mtle = NULL;
    }
}

/*
 * Invoke a wrapped callback. This should happen on the main thread.
 */
void moon_taskloader_handle_event(void *data)
{
    struct moon_taskloader_event *mtle = (struct moon_taskloader_event *) data;

    if (mtle->callback)
        mtle->callback(mtle->callback_data, mtle->extra_data);

    free_moon_taskloader_event(mtle);
}

/*---------------------------------------------------------------------------*/

/*
 * Count loaders in the linked list.
 *
 * Seems less error-prone than keeping a count variable in sync.
 */
static int count_active_loaders(void)
{
    int n = 0;
    List l;

    for (l = moon_taskloader_list; l; l = l->next)
        n++;

    return n;
}

/*
 * Allocate a new moon_taskloader_info struct.
 */
static struct moon_taskloader_info *create_moon_taskloader_info(void)
{
    struct moon_taskloader_info *mtli = (struct moon_taskloader_info *) calloc(sizeof (*mtli), 1);

    if (mtli)
        mtli->moon_taskloader_id = ++last_moon_taskloader_id;

    return mtli;
}

/*
 * Allocate a new moon_taskloader_info struct and add it to the load task list.
 */
static struct moon_taskloader_info *create_and_link_moon_taskloader_info(void)
{
    struct moon_taskloader_info *mtli = create_moon_taskloader_info();

    if (mtli)
        moon_taskloader_list = list_cons(mtli, moon_taskloader_list);

    return mtli;
}

/*
 * Clean up a moon_taskloader_info struct and associated resources.
 */
static void free_moon_taskloader_info(struct moon_taskloader_info *mtli)
{
    if (mtli)
    {
        free(mtli);
        mtli = NULL;
    }
}

/*
 * Remove a moon_taskloader_info from the transfer list and then free it.
 */
static void unlink_and_free_moon_taskloader_info(struct moon_taskloader_info *fi)
{
    if (fi)
    {
        /* First, remove from the linked list. */

        List l, p;

        for (p = NULL, l = moon_taskloader_list; l; p = l, l = l->next)
            if (l->data == fi)
            {
                if (p)
                    p->next = list_rest(l);
                else
                    moon_taskloader_list = list_rest(l);

                break;
            }

        /* Then, do clean up. */

        free_moon_taskloader_info(fi);
        fi = NULL;
    }
}

/*
 * Clean up all moon_taskloader_info structs and the linked list.
 *
 * This should only be called AFTER the moon taskloader thread has exited.
 */
static void free_all_moon_taskloader_infos(void)
{
    List l;

    for (l = moon_taskloader_list; l; l = list_rest(l))
    {
        free_moon_taskloader_info((struct moon_taskloader_info *) l->data);
        l->data = NULL;
    }

    moon_taskloader_list = NULL;
}

/*---------------------------------------------------------------------------*/

/*
 * Moon taskloader progress function.
 */
static int moon_taskloader_progress_func(void *clientp, double load_total, double load_now)
{
    struct moon_taskloader_info *mtli = (struct moon_taskloader_info *) clientp;

    if (mtli && mtli->callback.progress)
    {
        struct moon_taskloader_event *mtle = create_moon_taskloader_event();

        if (mtle)
        {
            mtle->callback      = mtli->callback.progress;
            mtle->callback_data = mtli->callback.data;
            mtle->extra_data    = create_extra_progress(load_total, load_now);

            moon_taskloader_dispatch_event(mtle);
        }
    }

    return 0;
}

/*
 * Progress all task loaders
 */
static void moon_taskloader_step(void)
{
    int done = 0;

    if (count_active_loaders())
    {
        for (List l = moon_taskloader_list; l && done == 0; l = l->next)
        {
            struct moon_taskloader_info *mtli = (struct moon_taskloader_info *) l->data;

            if (mtli)
            {
                int finished = -1;

                if (mtli->callback.execute)
                    finished = mtli->callback.execute(mtli, mtli->callback.data);

                if (mtli->callback.done)
                {
                    struct moon_taskloader_event *mtle = create_moon_taskloader_event();

                    if (mtle)
                    {
                        mtle->callback      = mtli->callback.done;
                        mtle->callback_data = mtli->callback.data;
                        mtle->extra_data    = create_extra_done(finished);

                        moon_taskloader_dispatch_event(mtle);
                    }

                    unlink_and_free_moon_taskloader_info(mtli);
                }

                done = 1;
                break;
            }
        }
    }
}

/*---------------------------------------------------------------------------*/

static SDL_mutex  *moon_taskloader_mutex;
static SDL_Thread *moon_taskloader_thread;

static SDL_atomic_t moon_taskloader_thread_running;

static int lock_hold_mutex = 0;

/*
 * Moon taskloader thread entry point.
 */
static int moon_taskloader_thread_func(void *data)
{
    log_printf("Starting moon taskloader thread\n");

    while (SDL_AtomicGet(&moon_taskloader_thread_running))
    {
        while (lock_hold_mutex) {}

        lock_hold_mutex = 1;
        SDL_LockMutex(moon_taskloader_mutex);
        moon_taskloader_step();
        SDL_UnlockMutex(moon_taskloader_mutex);
        lock_hold_mutex = 0;
    }

    log_printf("Stopping moon taskloader thread\n");

    return 0;
}

/*
 * Start the thread.
 */
static void moon_taskloader_thread_init(void)
{
    SDL_AtomicSet(&moon_taskloader_thread_running, 1);
    moon_taskloader_mutex = SDL_CreateMutex();
    moon_taskloader_thread = SDL_CreateThread(moon_taskloader_thread_func, "MoonTaskloader", NULL);
}

/*
 * Wait for thread to exit and do cleanup.
 */
static void moon_taskloader_thread_quit(void)
{
    SDL_AtomicSet(&moon_taskloader_thread_running, 0);

    SDL_WaitThread(moon_taskloader_thread, NULL);
    moon_taskloader_thread = NULL;

    SDL_DestroyMutex(moon_taskloader_mutex);
    moon_taskloader_mutex = NULL;
}

/*
 * Gain thread access to shared data.
 */
static int moon_taskloader_lock_mutex(void)
{
    while (lock_hold_mutex) {}

    /* Then, attempt to acquire mutex. */
    lock_hold_mutex = 1;
    return SDL_LockMutex(moon_taskloader_mutex);
}

/*
 * Give up thread access to shared.
 */
static int moon_taskloader_unlock_mutex(void)
{
    lock_hold_mutex = 0;
    return SDL_UnlockMutex(moon_taskloader_mutex);
}

/*---------------------------------------------------------------------------*/

static int moon_taskloader_was_init = 0;

/*
 * Initialize moon task loader.
 */
void moon_taskloader_init(void (*dispatch_event) (void *))
{
    if (moon_taskloader_was_init)
        moon_taskloader_quit();

    last_moon_taskloader_id = 0;

    moon_taskloader_dispatch_event = dispatch_event;

    moon_taskloader_thread_init();

    moon_taskloader_was_init = 1;
}

/*
 * Shut down moon task loader.
 */
void moon_taskloader_quit(void)
{
    if (!moon_taskloader_was_init) return;

    moon_taskloader_thread_quit();

    free_all_moon_taskloader_infos();

    last_moon_taskloader_id = 0;

    moon_taskloader_was_init = 0;
}

/*
 * Load SOL file into the taskloader.
 */
unsigned int moon_taskloader_load(const char *filename,
                                  struct moon_taskloader_callback callback)
{
    unsigned int moon_taskloader_id = 0;

    moon_taskloader_lock_mutex();

    struct moon_taskloader_info *mtli = create_and_link_moon_taskloader_info();

    if (mtli)
    {
        log_printf("Starting task loader %u\n", mtli->moon_taskloader_id);
        log_printf("Loading from %s\n",         filename);

        mtli->callback     = callback;
        mtli->filename     = strdup(filename);

        moon_taskloader_id = mtli->moon_taskloader_id;
    }

    moon_taskloader_unlock_mutex();

    return moon_taskloader_id;
}

/*---------------------------------------------------------------------------*/
