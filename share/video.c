/*
 * Copyright (C) 2023 Microsoft / Neverball authors
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

#include <assert.h>

#ifdef __EMSCRIPTEN__
#include <gl4esinit.h>
#endif

#if _WIN32 && __MINGW32__
#include <SDL3/SDL.h>
#else
#include <SDL.h>
#endif

#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
#include "console_control_gui.h"
#endif

#include "video.h"
#include "common.h"
#include "image.h"
#include "vec3.h"
#include "glext.h"
#include "config.h"
#include "gui.h"
#include "hmd.h"

#define ENABLE_MULTISAMPLE_SOLUTION
#define FPS_REALTIME

extern const char TITLE[];
extern const char ICON[];

struct video video;

/*---------------------------------------------------------------------------*/

/* Normally...... show the system cursor and hide the virtual cursor.        */
/* In HMD mode... show the virtual cursor and hide the system cursor.        */

void video_show_cursor()
{
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
    if (current_platform == PLATFORM_PC)
#endif
    {
#ifdef SWITCHBALL_GUI
        gui_set_cursor(1);
        SDL_ShowCursor(SDL_DISABLE);
#else
        if (hmd_stat())
        {
            gui_set_cursor(1);
            SDL_ShowCursor(SDL_DISABLE);
        }
        else
        {
            gui_set_cursor(0);
            SDL_ShowCursor(SDL_ENABLE);
        }
#endif
    }
#if !defined(__EMSCRIPTEN__) && NB_HAVE_PB_BOTH==1
    else
    {
        /* You won't be able to use the cursor, while using the
         * Nintendo Switch, PS4 or Xbox */
        gui_set_cursor(0);
        SDL_ShowCursor(SDL_DISABLE);
    }
#endif
}

/* When the cursor is to be hidden, make sure neither the virtual cursor     */
/* nor the system cursor is visible.                                         */

void video_hide_cursor()
{
    gui_set_cursor(0);
    SDL_ShowCursor(SDL_DISABLE);
}

/*---------------------------------------------------------------------------*/

static char snapshot_path[MAXSTR] = "";

static void snapshot_init(void)
{
    snapshot_path[0] = 0;
}

static void snapshot_prep(const char *path)
{
    if (path && *path)
        SAFECPY(snapshot_path, path);
}

static void snapshot_take(void)
{
    if (snapshot_path[0])
    {
        image_snap(snapshot_path);
        snapshot_path[0] = 0;
    }
}

void video_snap(const char *path)
{
    snapshot_prep(path);
}

/*---------------------------------------------------------------------------*/

static SDL_Window    *window;
static SDL_GLContext  context;

#if !_MSC_VER
static void set_window_icon(const char *filename)
{
    SDL_Surface *icon;

    if ((icon = load_surface(filename)))
    {
        SDL_SetWindowIcon(window, icon);
        free(icon->pixels);
        SDL_FreeSurface(icon);
    }
}
#endif

/*
 * Enter/exit fullscreen mode.
 */
int video_fullscreen(int f)
{
    int code = SDL_SetWindowFullscreen(window, f ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);

    if (code == 0)
        config_set_d(CONFIG_FULLSCREEN, f ? 1 : 0);
    else
        log_errorf("Failure to %s fullscreen (%s)\n", f ? "enter" : "exit", SDL_GetError() ? SDL_GetError() : "Unknown error");

    return (code == 0);
}

/*
 * Handle a window resize event.
 */
void video_resize(int window_w, int window_h)
{
    if (window)
    {
        /* Update window size (for mouse events). */

        video.window_w = window_w;
        video.window_h = window_h;

        /* User experience thing: don't save fullscreen size to config. */
        
        if (!config_get_d(CONFIG_FULLSCREEN))
        {
            config_set_d(CONFIG_WIDTH, video.window_w);
            config_set_d(CONFIG_HEIGHT, video.window_h);
        }

        /* Update drawable size (for rendering). */

        int wszW, wszH;
        
        SDL_GetWindowSize(window, &wszW, &wszH);
        SDL_GL_GetDrawableSize(window, &video.device_w, &video.device_h);
        
        video.scale_w = floorf((float) wszW / (float) video.device_w);
        video.scale_h = floorf((float) wszH / (float) video.device_h);

        video.device_scale = (float) video.device_h / (float) video.window_h;

        /* Update viewport. */

        glViewport(0, 0,
                   video.device_w * video.scale_w, video.device_h * video.scale_h);

        video.aspect_ratio = (float) video.device_w / (float) video.window_h;
    }
}

void video_set_window_size(int w, int h)
{
    /*
     * On Emscripten, SDL_SetWindowSize does all of these:
     *
     *   1) updates SDL's cached window->w and window->h values
     *   2) updates canvas.width and canvas.height (drawing buffer size)
     *   3) triggers a SDL_WINDOWEVENT_SIZE_CHANGED event, which updates our viewport/UI.
     */

     /*
      * BTW, for this to work with element.requestFullscreen(),
      * a change needs to be applied to the SDL2 Emscripten port:
      * https://github.com/emscripten-ports/SDL2/issues/138
      */

    SDL_SetWindowSize(window, w, h);
}

void video_set_display(int dpy)
{
    SDL_DisplayMode ddm;
    if (SDL_GetDesktopDisplayMode(dpy, &ddm) != 0)
    {
        log_errorf("No display connected!: %d\n", dpy);
        return;
    }

    SDL_Rect monitor_area_location;
    SDL_GetDisplayBounds(dpy, &monitor_area_location);

    int X = monitor_area_location.x + (ddm.w / 2) - (video.window_w / 2);
    int Y = monitor_area_location.y + (ddm.h / 2) - (video.window_h / 2);

    if (video.window_w > ddm.w ||
        video.window_h > ddm.h)
    {
        log_errorf("Window size exeeds the desktop resolution limit!: Current: %d/%d; Limit: %d/%d\n", video.window_w, video.window_h, ddm.w, ddm.h);
        video_set_window_size(ddm.w, ddm.h);
    }

    if (X - monitor_area_location.x > ddm.w + monitor_area_location.x / 2 ||
        Y - monitor_area_location.y > ddm.h + monitor_area_location.y / 2)
    {
        log_errorf("Window position exeeds the desktop resolution limit!: Current: %d/%d; Limit: %d/%d\n", X, Y, ddm.w, ddm.h);
        X = MAX((ddm.w / 2) - (video.window_w / 2), 0) + monitor_area_location.x;
        Y = MAX((ddm.h / 2) - (video.window_h / 2), 0) + monitor_area_location.y;
    }

    SDL_SetWindowPosition(window, X, Y);
}

int video_display(void)
{
    if (window)
        return SDL_GetWindowDisplayIndex(window);
    else
        return -1;
}

int video_init(void)
{
    if (!video_mode(config_get_d(CONFIG_FULLSCREEN),
                    config_get_d(CONFIG_WIDTH),
                    config_get_d(CONFIG_HEIGHT)))
    {
        log_errorf("Failure to create window (%s)\n", SDL_GetError() ? SDL_GetError() : "Unknown error");
        return 0;
    }

    return 1;
}

void video_quit(void)
{
    if (context)
    {
        SDL_GL_DeleteContext(context);
        context = NULL;
    }

#if NB_STEAM_API==0 && !defined(__EMSCRIPTEN__)
    if (window)
    {
        SDL_DestroyWindow(window);
        window = NULL;
    }
#endif

    hmd_free();
}

int video_mode(int f, int w, int h)
{
    int stereo   = config_get_d(CONFIG_STEREO)      ? 1 : 0;
    int stencil  = config_get_d(CONFIG_REFLECTION)  ? 1 : 0;
    int buffers  = config_get_d(CONFIG_MULTISAMPLE) ? 1 : 0;
    int samples  = config_get_d(CONFIG_MULTISAMPLE);
    int texture  = config_get_d(CONFIG_TEXTURES);
    int vsync    = config_get_d(CONFIG_VSYNC)       ? 1 : 0;
    int hmd      = config_get_d(CONFIG_HMD)         ? 1 : 0;
    int highdpi  = config_get_d(CONFIG_HIGHDPI)     ? 1 : 0;

    int num_displays = SDL_GetNumVideoDisplays();
    if (num_displays < 1)
    {
        log_errorf("No displays provided!\n");
        return 0;
    }

#if defined(__EMSCRIPTEN__)
    int dpy = config_get_d(CONFIG_DISPLAY);
#else
    int dpy =
        CLAMP(0, config_get_d(CONFIG_DISPLAY), num_displays - 1);
#endif

    SDL_DisplayMode ddm;
    if (SDL_GetDesktopDisplayMode(dpy, &ddm) != 0)
    {
        log_errorf("No display connected!: %d\n", dpy);
        return 0;
    }
    
    SDL_Rect monitor_area_location;
    SDL_GetDisplayBounds(dpy, &monitor_area_location);

    int X = monitor_area_location.x + (ddm.w / 2) - (w / 2);
    int Y = monitor_area_location.y + (ddm.h / 2) - (h / 2);

    if (w > ddm.w ||
        h > ddm.h)
    {
        log_errorf("Window size exeeds the desktop resolution limit!: Current: %d/%d; Limit: %d/%d\n", w, h, ddm.w, ddm.h);
        w = ddm.w;
        h = ddm.h;
    }

    if (X - monitor_area_location.x > ddm.w + monitor_area_location.x / 2 ||
        Y - monitor_area_location.y > ddm.h + monitor_area_location.y / 2)
    {
        log_errorf("Window position exeeds the desktop resolution limit during creation!: Current: %d/%d; Limit: %d/%d\n", X, Y, ddm.w, ddm.h);
        X = MAX((ddm.w / 2) - (w / 2), 0) + monitor_area_location.x;
        Y = MAX((ddm.h / 2) - (h / 2), 0) + monitor_area_location.y;
    }

    hmd_free();

    video_quit();

#if ENABLE_OPENGLES
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif
    
#ifndef ENABLE_HMD
    /* If the HMD is not ready, use without stereo. */
    config_set_d(CONFIG_STEREO, 0);
    stereo = 0;
#endif

    SDL_GL_SetAttribute(SDL_GL_STEREO,             stereo);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,       stencil);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, buffers);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, samples);

    /* Require 16-bit double buffer with 16-bit depth buffer. */

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE,     5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,   5);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,    5);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,  16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    /* Try to set the currently specified mode. */

    log_printf("Creating a window (%dx%d, %s)\n",
               w, h, (f ? "fullscreen" : "windowed"));

#if NB_STEAM_API==1 && !defined(__EMSCRIPTEN__)
    if (!window) {
#endif
        window = SDL_CreateWindow(TITLE, X, Y, MAX(w, 320), MAX(h, 240),
            SDL_WINDOW_OPENGL
            | SDL_WINDOW_ALLOW_HIGHDPI
#ifndef __EMSCRIPTEN__
#ifdef RESIZEABLE_WINDOW
            | SDL_WINDOW_RESIZABLE
            | (config_get_d(CONFIG_MAXIMIZED) ? SDL_WINDOW_MAXIMIZED : 0)
#endif
            | (f ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0)
#endif
        );
        
#ifdef RESIZEABLE_WINDOW
        if (config_get_d(CONFIG_MAXIMIZED))
            SDL_MaximizeWindow(window);
#endif
#if NB_STEAM_API==1 && !defined(__EMSCRIPTEN__)
    }
    else
    {
        SDL_SetWindowFullscreen(window, f ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        SDL_SetWindowPosition(window, X, Y);
        SDL_SetWindowSize(window, w, h);
    }
#endif

    if (window)
    {
#if _WIN32
        SDL_SetWindowMinimumSize(window, 800, 600);
#else
        SDL_SetWindowMinimumSize(window, 320, 240);
#endif

        if ((context = SDL_GL_CreateContext(window)))
        {
#ifdef __EMSCRIPTEN__
            initialize_gl4es();
#endif

            /* Check if the sampling and buffer values are NOT in negative. */

            if (buffers < 0) {
                log_errorf("Buffers cannot be negative!\n");

                if (context) {
                    SDL_GL_DeleteContext(context);
                    context = NULL;
                }

                /* Ignore initializing and exit programm. */

                return 0;
            }

            if (samples < 0) {
                log_errorf("Samples cannot be negative!\n");

                if (context) {
                    SDL_GL_DeleteContext(context);
                    context = NULL;
                }

                /* Ignore initializing and exit programm. */

                return 0;
            }

            int buf, smp;

            SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &buf);
            SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &smp);

            if (buf < buffers || smp < samples)
            {
                log_errorf("GL context does not meet minimum specifications!\n");
                SDL_GL_DeleteContext(context);
                context = NULL;
            }
        }
    }

    if (window && context)
    {
#if !_MSC_VER
        set_window_icon(ICON);
#endif

        log_printf("Created a window (%u, %dx%d, %s)\n",
                   SDL_GetWindowID(window),
                   video.window_w, video.window_h,
                   (f ? "fullscreen" : "windowed"));

        config_set_d(CONFIG_DISPLAY,    video_display());
        config_set_d(CONFIG_FULLSCREEN, f);

        SDL_GL_SetSwapInterval(vsync);

        if (!glext_init())
            return 0;

        video_resize(w, h);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        glEnable(GL_NORMALIZE);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_LIGHTING);
        glEnable(GL_BLEND);

#if !ENABLE_OPENGLES
        glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,
                      GL_SEPARATE_SPECULAR_COLOR);
#endif

        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthFunc(GL_LEQUAL);

        /* If GL supports multisample, and SDL got a multisample buffer... */

        if (glext_check_ext("ARB_multisample"))
        {
            SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &buffers);
            if (buffers) glEnable(GL_MULTISAMPLE);
        }

        /* Set up HMD display if requested. */

        if (hmd)
            hmd_init();

        /* Initialize screen snapshotting. */

        snapshot_init();

        video_show_cursor();

        /* Grab input immediately in HMD mode. */

        if (hmd_stat())
            SDL_SetWindowGrab(window, SDL_TRUE);

        /* Recenter the cursor back in full screen mode. */

        if (config_get_d(CONFIG_FULLSCREEN))
            SDL_WarpMouseInWindow(window,
                video.window_w / 2,
                video.window_h / 2);
        
        config_set_d(CONFIG_DISPLAY, dpy);
        config_set_d(CONFIG_WIDTH, video.window_w);
        config_set_d(CONFIG_HEIGHT, video.window_h);
        video.aspect_ratio = (float) video.window_w / video.window_h;

        config_save();
        return 1;
    }
#if NB_STEAM_API==0 && !defined(__EMSCRIPTEN__)
#if ENABLE_HMD
    /* If the mode failed, try it without stereo. */

    else if (stereo)
    {
        config_set_d(CONFIG_STEREO, 0);
        return video_mode(f, w, h);
    }
#endif

    /* If the mode failed, try decreasing the level of multisampling. */

    else if (buffers)
    {
        config_set_d(CONFIG_MULTISAMPLE, samples / 2);
        return video_mode(f, w, h);
    }

    /* If the mode failed, try decreasing the level of textures. */

    else if (texture != 2)
    {
        config_set_d(CONFIG_TEXTURES, 2);
        return video_mode(f, w, h);
    }

    /* If that mode failed, try it without reflections. */

    else if (stencil)
    {
        config_set_d(CONFIG_REFLECTION, 0);
        return video_mode(f, w, h);
    }

    /* If that mode failed, try it without v-sync. */

    else if (vsync)
    {
        config_set_d(CONFIG_VSYNC, 0);
        return video_mode(f, w, h);
    }

    /* If that mode failed, try it without background. */

    else if (config_get_d(CONFIG_BACKGROUND) == 1)
    {
        config_set_d(CONFIG_BACKGROUND, 0);
        return video_mode(f, w, h);
    }

    /* If that mode failed, try it without shadow. */

    else if (config_get_d(CONFIG_SHADOW) == 1)
    {
        config_set_d(CONFIG_SHADOW, 0);
        return video_mode(f, w, h);
    }

    /* If that mode failed, get the soloution. */

    else if (config_get_d(CONFIG_MULTISAMPLE) == 0 && window)
    {
        int solution_buf, solution_smp;
        SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &solution_buf);
        SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &solution_smp);

        if (solution_buf < solution_smp)
            config_set_d(CONFIG_MULTISAMPLE, solution_smp);
        else if (solution_buf > solution_smp)
            config_set_d(CONFIG_MULTISAMPLE, solution_buf);
        else
            config_set_d(CONFIG_MULTISAMPLE, solution_smp);

        assert(solution_buf > -1 && solution_smp > -1);

        return video_mode(f, w, h);
    }

    /* If THAT mode failed, punt. */
#else
#if defined(ENABLE_MULTISAMPLE_SOLUTION)
    /* Get the soloution first. */

    if (config_get_d(CONFIG_MULTISAMPLE) == 0 && window)
    {
        int solution_buf, solution_smp;
        SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &solution_buf);
        SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &solution_smp);

        if (solution_buf < solution_smp)
            config_set_d(CONFIG_MULTISAMPLE, solution_smp);
        else if (solution_buf > solution_smp)
            config_set_d(CONFIG_MULTISAMPLE, solution_buf);
        else
            config_set_d(CONFIG_MULTISAMPLE, solution_smp);

        assert(solution_buf > -1 && solution_smp > -1);

        return video_mode(f, w, h);
    }
#endif

    /* For some reasons, this may not work on oldest hardware. */

#endif

    return 0;
}

int video_mode_auto_config(int f, int w, int h)
{
    int stereo   = config_get_d(CONFIG_STEREO)      ? 1 : 0;
    int hmd      = config_get_d(CONFIG_HMD)         ? 1 : 0;

    int num_displays = SDL_GetNumVideoDisplays();
    if (num_displays < 1)
    {
        log_errorf("No displays provided!\n");
        return 0;
    }

#if defined(__EMSCRIPTEN__)
    int dpy = config_get_d(CONFIG_DISPLAY);
#else
    int dpy =
        CLAMP(0, config_get_d(CONFIG_DISPLAY), SDL_GetNumVideoDisplays() - 1);
#endif

    SDL_DisplayMode ddm;
    if (SDL_GetDesktopDisplayMode(dpy, &ddm) != 0)
    {
        log_errorf("No display connected!: %d\n", dpy);
        return 0;
    }

    SDL_Rect monitor_area_location;
    SDL_GetDisplayBounds(dpy, &monitor_area_location);

    int X = monitor_area_location.x + (ddm.w / 2) - (w / 2);
    int Y = monitor_area_location.y + (ddm.h / 2) - (h / 2);

    if (w > ddm.w ||
        h > ddm.h)
    {
        log_errorf("Window size exeeds the desktop resolution limit!: Current: %d/%d; Limit: %d/%d\n", w, h, ddm.w, ddm.h);
        w = ddm.w;
        h = ddm.h;
    }

    if (X - monitor_area_location.x > ddm.w + monitor_area_location.x / 2 ||
        Y - monitor_area_location.y > ddm.h + monitor_area_location.y / 2)
    {
        log_errorf("Window position exeeds the desktop resolution limit during creation!: Current: %d/%d; Limit: %d/%d\n", X, Y, ddm.w, ddm.h);
        X = MAX((ddm.w / 2) - (w / 2), 0) + monitor_area_location.x;
        Y = MAX((ddm.h / 2) - (h / 2), 0) + monitor_area_location.y;
    }

    hmd_free();

    video_quit();

#if ENABLE_OPENGLES
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif

    SDL_GL_SetAttribute(SDL_GL_STEREO,             stereo);

    int auto_stencils = 1;
    int stencil_ok = SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, auto_stencils);

    int auto_samples = 16;
    int smpbuf_ok = SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, auto_samples ? 1 : 0);
    int smp_ok = SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, auto_samples);

    /* Require 16-bit double buffer with 16-bit depth buffer. */

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    log_printf("Creating a window (%dx%d, %s (auto configuration))\n",
        w, h, (f ? "fullscreen" : "windowed"));

#if NB_STEAM_API==1 && !defined(__EMSCRIPTEN__)
    if (!window) {
#endif
        window = SDL_CreateWindow(TITLE, X, Y, MAX(w, 320), MAX(h, 240),
            SDL_WINDOW_OPENGL
            | SDL_WINDOW_ALLOW_HIGHDPI
#ifndef __EMSCRIPTEN__
#ifdef RESIZEABLE_WINDOW
            | SDL_WINDOW_RESIZABLE
            | (config_get_d(CONFIG_MAXIMIZED) ? SDL_WINDOW_MAXIMIZED : 0)
#endif
            | (f ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0)
#endif
        );

#ifdef RESIZEABLE_WINDOW
        if (config_get_d(CONFIG_MAXIMIZED))
            SDL_MaximizeWindow(window);
#endif
#if NB_STEAM_API==1 && !defined(__EMSCRIPTEN__)
    }
    else
    {
        SDL_SetWindowFullscreen(window, f ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        SDL_SetWindowPosition(window, X, Y);
        SDL_SetWindowSize(window, w, h);
    }
#endif

    int got_geforce = 0;

    while (auto_stencils > 0 && got_geforce == 0)
    {
        if (stencil_ok == 0)
        {
            if ((context = SDL_GL_CreateContext(window)))
            {
                int stn;

                if (SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &stn) == -1)
                    return 0;

                if (stn >= auto_stencils)
                {
                    SDL_GL_DeleteContext(context);
                    context = NULL;
                    config_set_d(CONFIG_REFLECTION, auto_stencils);
                    got_geforce = 1;
                }
                else if (auto_stencils != 0)
                {
                    SDL_GL_DeleteContext(context);
                    context = NULL;
                    auto_stencils /= 2;
                    if (SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, auto_stencils) == -1)
                        return 0;
                }
                else
                    return 0;
            }
        }
        else
            return 0;
    }

    got_geforce = 0;
    while (auto_samples > 0 && got_geforce == 0)
    {
        if (smpbuf_ok == 0 && smp_ok == 0)
        {
            if ((context = SDL_GL_CreateContext(window)))
            {
                int buf, smp;

                if (SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &buf) == -1 ||
                    SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &smp) == -1)
                    return 0;

                if (buf >= 1 && smp >= auto_samples)
                {
                    SDL_GL_DeleteContext(context);
                    context = NULL;
                    config_set_d(CONFIG_MULTISAMPLE, auto_samples);
                    got_geforce = 1;
                }
                else if (auto_samples != 0)
                {
                    SDL_GL_DeleteContext(context);
                    context = NULL;
                    auto_samples /= 2;
                    if (SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, auto_samples) == -1
                        && SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, auto_samples ? 1 : 0) == -1)
                        return 0;
                }
                else
                    return 0;
            }
        }
        else
            return 0;
    }

    if (window)
    {
#if _WIN32
        SDL_SetWindowMinimumSize(window, 600, 600);
#else
        SDL_SetWindowMinimumSize(window, 320, 240);
#endif

        config_set_d(CONFIG_HIGHDPI, 1);
        if ((context = SDL_GL_CreateContext(window)))
        {
#ifdef __EMSCRIPTEN__
            initialize_gl4es();
#endif
            int buf, smp;

            SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &buf);
            SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &smp);

            if (buf < (auto_samples > 0 ? 1 : 0) || smp < auto_samples)
            {
                log_errorf("GL context does not meet minimum specifications!\n");
                SDL_GL_DeleteContext(context);
                context = NULL;
                return 0;
            }
        }
    }

    if (window && context)
    {
#if !_MSC_VER
        set_window_icon(ICON);
#endif

        log_printf("Created a window (%u, %dx%d, %s (auto configuration))\n",
            SDL_GetWindowID(window),
            video.window_w, video.window_h,
            (f ? "fullscreen" : "windowed"));

        config_set_d(CONFIG_DISPLAY, video_display());
        config_set_d(CONFIG_FULLSCREEN, f);

        if (SDL_GL_SetSwapInterval(1) == -1) return 0;
        config_set_d(CONFIG_VSYNC, 1);

        if (!glext_init())
            return 0;

        video_resize(w, h);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        glEnable(GL_NORMALIZE);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_LIGHTING);
        glEnable(GL_BLEND);

#if !ENABLE_OPENGLES
        glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,
            GL_SEPARATE_SPECULAR_COLOR);
#endif

        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthFunc(GL_LEQUAL);

        /* If GL supports multisample, and SDL got a multisample buffer... */

        if (glext_check_ext("ARB_multisample"))
        {
            int buffers;
            SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &buffers);
            if (buffers) glEnable(GL_MULTISAMPLE);
        }

        /* Set up HMD display if requested. */

        if (hmd)
            hmd_init();

        /* Initialize screen snapshotting. */

        snapshot_init();

        video_show_cursor();

        /* Grab input immediately in HMD mode. */

        if (hmd_stat())
            SDL_SetWindowGrab(window, SDL_TRUE);

        /* Set it back for recommended configurations. */

        config_set_d(CONFIG_TEXTURES, 1);
        config_set_d(CONFIG_SHADOW, 1);
        config_set_d(CONFIG_BACKGROUND, 1);
#ifdef GL_GENERATE_MIPMAP_SGIS
        config_set_d(CONFIG_MIPMAP, 1);
#endif
#if defined(GL_TEXTURE_MAX_ANISOTROPY_EXT) && GL_TEXTURE_MAX_ANISOTROPY_EXT != 0 && GL_TEXTURE_MAX_ANISOTROPY_EXT != -1
        config_set_d(CONFIG_ANISO, 2);
#endif

        /* Recenter the cursor back in full screen mode. */

        if (config_get_d(CONFIG_FULLSCREEN))
        SDL_WarpMouseInWindow(window,
            video.window_w / 2,
            video.window_h / 2);

        config_set_d(CONFIG_DISPLAY, dpy);
        config_set_d(CONFIG_WIDTH, video.window_w);
        config_set_d(CONFIG_HEIGHT, video.window_h);
        video.aspect_ratio = (float) video.window_w / video.window_h;

        config_save();

        return 1;
    }

#if defined(ENABLE_MULTISAMPLE_SOLUTION)
    /* Get the soloution first. */

    if (config_get_d(CONFIG_MULTISAMPLE) == 0 && window)
    {
        int solution_buf, solution_smp;
        SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &solution_buf);
        SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &solution_smp);

        if (solution_buf < solution_smp)
            config_set_d(CONFIG_MULTISAMPLE, solution_smp);
        else if (solution_buf > solution_smp)
            config_set_d(CONFIG_MULTISAMPLE, solution_buf);
        else
            config_set_d(CONFIG_MULTISAMPLE, solution_smp);

        assert(solution_buf > -1 && solution_smp > -1);

        return video_mode(f, w, h);
    }
#endif
    /* For some reasons, this may not work on oldest hardware. */

    return 0;
}

/*---------------------------------------------------------------------------*/

static float ms     = 0;
static int   fps    = 0;
static int   last   = 0;
static int   ticks  = 0;
static int   frames = 0;

int  video_perf(void)
{
    return fps;
}

void video_swap(void)
{
    int dt;

    if (hmd_stat())
        hmd_swap();

    /* Take a screenshot of the complete back buffer and swap it. */

    snapshot_take();

    SDL_GL_SwapWindow(window);

    /* Accumulate time passed and frames rendered. */

    dt = (int) SDL_GetTicks() - last;

    last   += dt;

#ifdef FPS_REALTIME
    /* Compute frame time and frames-per-second stats. */

    fps = 1 / (0.001f * dt);
    ms = 0.001f * dt;

#if NB_STEAM_API==0 && !defined(LOG_NO_STATS)
    /* Output statistics if configured. */

    if (config_get_d(CONFIG_STATS))
        fprintf(stdout, "%4d %8.4f\n", fps, (double) ms);
#endif
#else
    frames += 1;
    ticks += dt;

    /* Average over 250ms. */

    if (ticks > 1000)
    {
        /* Round the frames-per-second value to the nearest integer. */

        double k = 1000.0 * frames / ticks;
        double f = floor(k);
        double c = ceil (k);

        /* Compute frame time and frames-per-second stats. */

        fps = (int) ((c - k < k - f) ? c : f);
        ms  = (float) ticks / (float) frames;

        /* Reset the counters for the next update. */

        frames = 0;
        ticks  = 0;

#if NB_STEAM_API==0 && !defined(LOG_NO_STATS)
        /* Output statistics if configured. */

        if (config_get_d(CONFIG_STATS))
            fprintf(stdout, "%4d %8.4f\n", fps, (double) ms);
#endif
    }
#endif
}

/*---------------------------------------------------------------------------*/

static int grabbed = 0;

void video_set_grab(int w)
{
    if (w)
    {
        SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);

        SDL_WarpMouseInWindow(window,
                              video.window_w / 2,
                              video.window_h / 2);

        SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
    }

    SDL_SetRelativeMouseMode(SDL_TRUE);
    SDL_SetWindowGrab(window, SDL_TRUE);
    video_hide_cursor();

    grabbed = 1;
}

void video_clr_grab(void)
{
    SDL_SetRelativeMouseMode(SDL_FALSE);

    /* Never release the grab in HMD mode. */

    if (!hmd_stat())
        SDL_SetWindowGrab(window, SDL_FALSE);

    video_show_cursor();

    grabbed = 0;
}

int  video_get_grab(void)
{
    return grabbed;
}

/*---------------------------------------------------------------------------*/

int viewport_wireframe = 0;
int wireframe_splitview = 0;
int splitview_crossed = 0;

int render_fill_overlay = 0;
int render_line_overlay = 0;
int render_left_viewport = 0;
int render_right_viewport = 0;

void video_set_wire(int wire)
{
#if !ENABLE_OPENGLES
    wireframe_splitview = 0;
    if (wire == 4)
    {
        viewport_wireframe = 4;
    }
    if (wire == 3)
    {
        viewport_wireframe = 3;
    }
    else if (wire == 2)
    {
        viewport_wireframe = 2;
        wireframe_splitview = 1;
    }
    else if (wire == 1)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);
        viewport_wireframe = 1;
        glViewport(0, 0, video.device_w * video.scale_w, video.device_h * video.scale_h);
    }
    else
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_LIGHTING);
        viewport_wireframe = 0;
        glViewport(0, 0, video.device_w * video.scale_w, video.device_h * video.scale_h);
    }
#endif
}

void video_render_fill_or_line(int lined)
{
#if !ENABLE_OPENGLES
    render_fill_overlay = 0;
    render_line_overlay = 0;

    render_left_viewport = 0;
    render_right_viewport = 0;

    if (viewport_wireframe == 4)
    {
        if (lined)
        {
            render_fill_overlay = 0;
            render_line_overlay = 1;
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_LIGHTING);
            glColor4f(1.f, 1.f, 1.f, 1.f);
        }
        else
        {
            render_line_overlay = 0;
            render_fill_overlay = 1;
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_TEXTURE_2D);
            glEnable(GL_LIGHTING);
            glColor4f(.5f, .5f, .5f, .5f);
        }
    }
    else if (lined && viewport_wireframe == 2 || viewport_wireframe == 3)
    {
        render_line_overlay = 0;
        render_fill_overlay = 0;
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);
        if (viewport_wireframe == 2)
        {
            render_right_viewport = 1;
            glViewport((int) video.device_w / 2, 0, (video.device_w / 2) * video.scale_w, video.device_h * video.scale_h);
        }
        else
            glViewport(0, 0, video.device_w * video.scale_w, video.device_h * video.scale_h);
    }
    else if (viewport_wireframe == 2 || viewport_wireframe == 3)
    {
        render_line_overlay = 0;
        render_fill_overlay = 0;

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_LIGHTING);
        if (viewport_wireframe == 2)
        {
            render_left_viewport = 1;
            glViewport(0, 0, (video.device_w / 2) * video.scale_w, video.device_h * video.scale_h);
        }
        else
            glViewport(0, 0, video.device_w * video.scale_w, video.device_h * video.scale_h);
    }
    else if (viewport_wireframe == 0)
    {
        render_line_overlay = 0;
        render_fill_overlay = 0;

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_LIGHTING);
        glViewport(0, 0, video.device_w * video.scale_w, video.device_h * video.scale_h);
    }
#endif
}

void video_toggle_wire(void)
{
    viewport_wireframe++;
    if (viewport_wireframe > 3)
        viewport_wireframe = 0;

    video_set_wire(viewport_wireframe);
}

void video_calc_view(float *M, const float *c,
                               const float *p,
                               const float *u)
{
    float x[3];
    float y[3];
    float z[3];

    v_sub(z, p, c);
    v_nrm(z, z);
    v_crs(x, u, z);
    v_nrm(x, x);
    v_crs(y, z, x);

    m_basis(M, x, y, z);
}

/*---------------------------------------------------------------------------*/

void video_push_persp(float fov, float n, float f)
{
    //glPushMatrix();
    if (hmd_stat())
        hmd_persp(n, f);
    else
    {
        GLfloat m[16];

        GLfloat r = fov / 2 * V_PI / 180;
        GLfloat s = fsinf(r);
        GLfloat c = fcosf(r) / s;

        GLfloat a = ((GLfloat) video.device_w /
                     (GLfloat) video.device_h);

        if (viewport_wireframe == 2)
            a = ((GLfloat) (video.device_w / 2) /
                 (GLfloat) video.device_h);

        glMatrixMode(GL_PROJECTION);
        {
            m[0 ] = c / a;
            m[1 ] = 0.0f;
            m[2 ] = 0.0f;
            m[3 ] = 0.0f;

            m[4 ] = 0.0f;
            m[5 ] = c;
            m[6 ] = 0.0f;
            m[7 ] = 0.0f;

            m[8 ] = 0.0f;
            m[9 ] = 0.0f;
            m[10] = -(f + n) / (f - n);
            m[11] = -1.0f;

            m[12] = 0.0f;
            m[13] = 0.0f;
            m[14] = -2.0f * n * f / (f - n);
            m[15] = 0.0f;

            glLoadMatrixf(m);
        }

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    }
}

void video_push_ortho(void)
{
    //glPushMatrix();

    if (hmd_stat())
        hmd_ortho();
    else
    {
        GLfloat w = (GLfloat) video.device_w;
        GLfloat h = (GLfloat) video.device_h;

        if (viewport_wireframe == 2)
            w /= 2;

        glMatrixMode(GL_PROJECTION);

        glLoadIdentity();
#if defined(__EMSCRIPTEN__)
        glOrtho(0.0, w, h, 0.0f, -1.0f, 1.0f);
#else
        glOrtho_(0.0, w, 0.0, h, -1.0, +1.0);
#endif

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        if (viewport_wireframe == 2 && render_right_viewport)
            glTranslatef(-w, 0, 0);
    }
}

void video_pop_matrix(void)
{
    //glPopMatrix();
}

void video_clear(void)
{
    GLbitfield bufferBit = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;

    if (config_get_d(CONFIG_REFLECTION))
        bufferBit |= GL_STENCIL_BUFFER_BIT;

    glClear(bufferBit);
}

/*---------------------------------------------------------------------------*/
