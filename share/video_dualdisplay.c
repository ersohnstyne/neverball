/*
 * Copyright (C) 2025 Microsoft / Neverball authors / Jānis Rūcis
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

/*
 * HACK: Remembering the code file differences:
 * Developers  who  programming  C++  can see more bedrock declaration
 * than C.  Developers  who  programming  C  can  see  few  procedural
 * declaration than  C++.  Keep  in  mind  when making  sure that your
 * extern code must associated. The valid file types are *.c and *.cpp,
 * so it's always best when making cross C++ compiler to keep both.
 * - Ersohn Styne
 */

#if __cplusplus
#include <vcruntime_exception.h>
#endif

#ifdef __EMSCRIPTEN__
#include <gl4esinit.h>
#endif

#if _WIN32 && __MINGW32__
#include <SDL2/SDL.h>
#elif _WIN32 && _MSC_VER
#include <SDL.h>
#elif _WIN32
#error Security compilation error: No target include file in path for Windows specified!
#else
#include <SDL.h>
#endif

#if __cplusplus
extern "C" {
#endif

/*
 * HACK: Used with console version
 */
#include "console_control_gui.h"

#include "dbg_config.h"

#include "video.h"
#include "video_dualdisplay.h"

#include "common.h"
#include "image.h"
#include "vec3.h"
#include "glext.h"
#include "config.h"
#include "gui.h"
#include "hmd.h"
#include "log.h"
#if __cplusplus
}
#endif

struct video_dualdisplay video_ddpy;

/*---------------------------------------------------------------------------*/

static char snapshot_path[MAXSTR] = "";

static void snapshot_init(void)
{
#if ENABLE_DUALDISPLAY==1 && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__SWITCH__)
    snapshot_path[0] = 0;
#endif
}

static void snapshot_prep(const char *path)
{
#if ENABLE_DUALDISPLAY==1 && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__SWITCH__)
    if (path && *path)
        SAFECPY(snapshot_path, path);
#endif
}

static void snapshot_take(void)
{
#if ENABLE_DUALDISPLAY==1 && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__SWITCH__)
    if (snapshot_path[0])
    {
        image_snap(snapshot_path);
        snapshot_path[0] = 0;
    }
#endif
}

#if __cplusplus
extern "C"
#endif
void video_dualdisplay_snap(const char *path)
{
#if ENABLE_DUALDISPLAY==1 && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__SWITCH__)
    snapshot_prep(path);
#endif
}

/*---------------------------------------------------------------------------*/

#if !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__SWITCH__)
static SDL_Window    *window_ddpy;
static SDL_GLContext  context_ddpy;
#endif

#if !_MSC_VER && !defined(__APPLE__)
static void set_window_icon(const char *filename)
{
#if ENABLE_DUALDISPLAY==1 && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__SWITCH__)
    if (!window) return;

    SDL_Surface *icon;

    if ((icon = load_surface(filename)))
    {
        SDL_SetWindowIcon(window, icon);

        free(icon->pixels);
        icon->pixels = NULL;

        SDL_FreeSurface(icon);
    }
#endif
}
#endif

/*
 * Enter/exit fullscreen mode.
 */
#if __cplusplus
extern "C"
#endif
int video_dualdisplay_fullscreen(int f)
{
#if ENABLE_DUALDISPLAY==1 && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__SWITCH__)
    if (!window_ddpy) return 0;

    int code = SDL_SetWindowFullscreen(window_ddpy,
                                       f ? SDL_WINDOW_FULLSCREEN_DESKTOP :
                                           0);

    if (code == 0)
        config_set_d(CONFIG_FULLSCREEN, f ? 1 : 0);
    else
        log_errorf("Failure to %s fullscreen (%s)\n", f ? "enter" : "exit",
                   GAMEDBG_GETSTRERROR_CHOICES_SDL);

    return (code == 0);
#else
    return 0;
#endif
}

/*
 * Handle a window resize event.
 */
#if __cplusplus
extern "C"
#endif
void video_dualdisplay_resize(int window_w, int window_h)
{
#if ENABLE_DUALDISPLAY==1 && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__SWITCH__)
    if (window_ddpy)
    {
        /* Update window size (for mouse events). */

        video_ddpy.ddpy_window_w = window_w;
        video_ddpy.ddpy_window_h = window_h;

        /* User experience thing: don't save fullscreen size to config. */

        if (!config_get_d(CONFIG_FULLSCREEN))
        {
            config_set_d(CONFIG_WIDTH,  video_ddpy.ddpy_window_w);
            config_set_d(CONFIG_HEIGHT, video_ddpy.ddpy_window_h);
        }

        /* Update drawable size (for rendering). */

        int wszW, wszH;

        SDL_GetWindowSize(window_ddpy, &wszW, &wszH);
        SDL_GetWindowSizeInPixels(window_ddpy, &video_ddpy.ddpy_device_w, &video_ddpy.ddpy_device_h);

        video_ddpy.ddpy_scale_w = floorf((float) wszW / (float) video_ddpy.ddpy_device_w);
        video_ddpy.ddpy_scale_h = floorf((float) wszH / (float) video_ddpy.ddpy_device_h);

        video_ddpy.ddpy_device_scale = (float) video_ddpy.ddpy_device_h / (float) video_ddpy.ddpy_window_h;

        /* Update viewport. */

        glViewport(0, 0,
                   video_ddpy.ddpy_device_w * video_ddpy.ddpy_scale_w,
                   video_ddpy.ddpy_device_h * video_ddpy.ddpy_scale_h);

        video_ddpy.ddpy_aspect_ratio = (float) video_ddpy.ddpy_device_w / (float) video_ddpy.ddpy_window_h;
    }
#endif
}

#if __cplusplus
extern "C"
#endif
void video_dualdisplay_set_window_size(int w, int h)
{
#if ENABLE_DUALDISPLAY==1 && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__SWITCH__)
    SDL_SetWindowSize(window_ddpy, w, h);
#endif
}

#if __cplusplus
extern "C"
#endif
void video_dualdisplay_set_display(int dpy)
{
#if ENABLE_DUALDISPLAY==1 && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__SWITCH__)
    SDL_DisplayMode ddm;
    if (SDL_GetDesktopDisplayMode(dpy, &ddm) != 0)
    {
        log_errorf("No display connected!: %d\n", dpy);
        return;
    }

    SDL_Rect monitor_area_location;
    SDL_GetDisplayBounds(dpy, &monitor_area_location);

    int X = monitor_area_location.x + (ddm.w / 2) - (video_ddpy.ddpy_window_w / 2);
    int Y = monitor_area_location.y + (ddm.h / 2) - (video_ddpy.ddpy_window_h / 2);

    if (video_ddpy.ddpy_window_w > ddm.w ||
        video_ddpy.ddpy_window_h > ddm.h)
    {
        log_errorf("Window size exeeds the desktop resolution limit!: Current: %d/%d; Limit: %d/%d\n",
                   video_ddpy.ddpy_window_w, video_ddpy.ddpy_window_h, ddm.w, ddm.h);
        video_set_window_size(ddm.w, ddm.h);
    }

    if (X - monitor_area_location.x > ddm.w + monitor_area_location.x / 2 ||
        Y - monitor_area_location.y > ddm.h + monitor_area_location.y / 2)
    {
        log_errorf("Window position exeeds the desktop resolution limit!: Current: %d/%d; Limit: %d/%d\n",
                   X, Y, ddm.w, ddm.h);
        X = MAX((ddm.w / 2) - (video_ddpy.ddpy_window_w / 2), 0) + monitor_area_location.x;
        Y = MAX((ddm.h / 2) - (video_ddpy.ddpy_window_h / 2), 0) + monitor_area_location.y;
    }

    SDL_SetWindowPosition(window_ddpy, X, Y);
#endif
}

#if __cplusplus
extern "C"
#endif
int video_dualdisplay_display(void)
{
#if ENABLE_DUALDISPLAY==1 && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__SWITCH__)
    if (window_ddpy)
        return SDL_GetWindowDisplayIndex(window_ddpy);
    else
#endif
        return -1;
}

#if __cplusplus
extern "C"
#endif
int video_dualdisplay_init(void)
{
#if ENABLE_DUALDISPLAY==1 && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__SWITCH__)
    return video_dualdisplay_mode(config_get_d(CONFIG_FULLSCREEN),
                                  config_get_d(CONFIG_WIDTH),
                                  config_get_d(CONFIG_HEIGHT));
#else
    return 0;
#endif
}

#if __cplusplus
extern "C"
#endif
void video_dualdisplay_quit(void)
{
#if ENABLE_DUALDISPLAY==1 && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__SWITCH__)
    if (context_ddpy)
    {
        SDL_GL_DeleteContext(context_ddpy);
        context_ddpy = NULL;
    }

//#if NB_STEAM_API==0 && !defined(__EMSCRIPTEN__)
    if (window_ddpy)
    {
        SDL_DestroyWindow(window_ddpy);
        window_ddpy = NULL;
    }
//#endif
#endif
}

int  video_dualdisplay_is_init(void)
{
#if !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__SWITCH__)
    return window_ddpy && context_ddpy;
#else
    return 0;
#endif
}

int video_dualdisplay_mode(int f, int w, int h)
{
#if ENABLE_DUALDISPLAY==1 && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__SWITCH__)
#if ENABLE_OPENGLES
    int init_gles = 1;
#endif

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

    int dpy = CLAMP(0, config_get_d(CONFIG_DISPLAY) + 1, num_displays - 1);

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
        log_errorf("Window size exeeds the desktop resolution limit!: Current: %d/%d; Limit: %d/%d\n",
                   w, h, ddm.w, ddm.h);
        w = ddm.w;
        h = ddm.h;
    }

    if (X - monitor_area_location.x > ddm.w + monitor_area_location.x / 2 ||
        Y - monitor_area_location.y > ddm.h + monitor_area_location.y / 2)
    {
        log_errorf("Window position exeeds the desktop resolution limit during creation!: Current: %d/%d; Limit: %d/%d\n",
                   X, Y, ddm.w, ddm.h);
        X = MAX((ddm.w / 2) - (w / 2), 0) + monitor_area_location.x;
        Y = MAX((ddm.h / 2) - (h / 2), 0) + monitor_area_location.y;
    }

    video_dualdisplay_quit();

#if ENABLE_OPENGLES
    if (init_gles) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    }
    else
    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    }
#endif

    SDL_GL_SetAttribute(SDL_GL_STEREO,             stereo);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,       stencil);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, buffers);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, samples);

    /*
     * Optional 16-bit double buffer with 16-bit depth buffer.
     *
     * Default RGB size: 2 - Either 5 (16-bit) or 8 (32-bit)
     * Default depth size: 8 - Either 8 or 16
     */

    int rgb_size_fixed = 2;

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   rgb_size_fixed);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, rgb_size_fixed);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  rgb_size_fixed);

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,   8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    log_printf("Creating a window (dualdisplay) (%dx%d, %s)\n",
               w, h, (f ? "fullscreen" : "windowed"));

#if NB_STEAM_API==1 && !defined(__EMSCRIPTEN__)
    if (!window) {
#endif
#if __cplusplus
        try {
#endif
            if (w && h)
            {
                window_ddpy = SDL_CreateWindow("Neverball Secondary Display", X, Y, MAX(w, 320), MAX(h, 240),
                    SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI
#ifndef __EMSCRIPTEN__
#ifdef RESIZEABLE_WINDOW
                    | SDL_WINDOW_RESIZABLE
                    | (config_get_d(CONFIG_MAXIMIZED) ? SDL_WINDOW_MAXIMIZED : 0)
#endif
                    | (f ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0)
#endif
                );
            }

#if _WIN32 && _MSC_VER
            GAMEDBG_CHECK_SEGMENTATIONS_BOOL(UNREFERENCED_PARAMETER(0));
#endif

#ifdef RESIZEABLE_WINDOW
            if (config_get_d(CONFIG_MAXIMIZED))
                SDL_MaximizeWindow(window_ddpy);
#endif
#if __cplusplus
        }
        catch (const std::exception& xO) {
            log_errorf("Failure to create window!: Exception caught! (%s)\n", xO.what());
            return 0;
        }
        catch (const char* xS) {
            log_errorf("Failure to create window!: Exception caught! (%s)\n", xS);
            return 0;
        }
        catch (...) {
            log_errorf("Failure to create window!: Exception caught! (Unknown type)\n");
            return 0;
        }
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

    if (window_ddpy) {
#if _WIN32
        SDL_SetWindowMinimumSize(window_ddpy, 800, 600);
#else
        SDL_SetWindowMinimumSize(window_ddpy, 320, 240);
#endif

        if ((context_ddpy = SDL_GL_CreateContext(window_ddpy)))
        {
            /*
             * Check whether the sampling and buffer values are
             * NOT in negative values in the multisampling settings.
             */

            if (buffers < 0)
            {
                log_errorf("Buffers cannot be negative!\n");
                SDL_GL_DeleteContext(context_ddpy);
                context_ddpy = NULL;
            }

            if (samples < 0)
            {
                log_errorf("Samples cannot be negative!\n");
                SDL_GL_DeleteContext(context_ddpy);
                context_ddpy = NULL;
            }

            int buf, smp;

            SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &buf);
            SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &smp);

            if (buf < buffers || smp < samples)
            {
                log_errorf("GL context does not meet minimum specifications!\n");
                SDL_GL_DeleteContext(context_ddpy);
                context_ddpy = NULL;
            }
        }
    }

    if (window_ddpy && context_ddpy)
    {
#if !_MSC_VER && !defined(__APPLE__) && !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && \
    !defined(__SWITCH__)
        set_window_icon(ICON);
#endif

        log_printf("Created a window (dualdisplay) (%u, %dx%d, %s)\n",
                   SDL_GetWindowID(window_ddpy),
                   w, h,
                   (f ? "fullscreen" : "windowed"));

        video_dualdisplay_resize(w, h);
        glClearColor(0.0f, 0.13f, 0.33f, 0.0f);

        glEnable(GL_NORMALIZE);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);

#if !ENABLE_OPENGLES
        glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,
                      GL_SEPARATE_SPECULAR_COLOR);
#endif

        glPixelStorei(GL_PACK_ALIGNMENT,   1);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthFunc(GL_LEQUAL);

        if (glext_check_ext("ARB_multisample"))
        {
            SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &buffers);
            if (buffers) glEnable(GL_MULTISAMPLE);
        }
        else
        {
            glDisable(GL_MULTISAMPLE);
            config_set_d(CONFIG_MULTISAMPLE, 0);
        }

        snapshot_init();

        video_ddpy.ddpy_aspect_ratio = (float) video_ddpy.ddpy_window_w / video_ddpy.ddpy_window_h;

        return 1;
    }
#endif

    return 0;
}

/*---------------------------------------------------------------------------*/

#if __cplusplus
extern "C"
#endif
void video_dualdisplay_swap(void)
{
#if ENABLE_DUALDISPLAY==1 && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__SWITCH__)
    snapshot_take();
    SDL_GL_SwapWindow(window_ddpy);
#endif
}

/*---------------------------------------------------------------------------*/

#if __cplusplus
extern "C"
#endif
void video_dualdisplay_set_current(void)
{
#if ENABLE_DUALDISPLAY==1 && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__SWITCH__)
    if (!window_ddpy || !context_ddpy) return;

    //SDL_GL_MakeCurrent(window_ddpy, context_ddpy);
#endif
}

#if __cplusplus
extern "C"
#endif
void video_dualdisplay_clear(void)
{
#if ENABLE_DUALDISPLAY==1 && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__SWITCH__)
    GLbitfield bufferBit = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;

    if (config_get_d(CONFIG_REFLECTION))
        bufferBit |= GL_STENCIL_BUFFER_BIT;

    glClear(bufferBit);
#endif
}

/*---------------------------------------------------------------------------*/
