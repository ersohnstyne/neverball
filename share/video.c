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
#ifndef NDEBUG
#include <assert.h>
#elif defined(_MSC_VER) && defined(_AFXDLL)
#include <afx.h>
/**
 * HACK: assert() for Microsoft Windows Apps in Release builds
 * will be replaced to VERIFY() - Ersohn Styne
 */
#define assert VERIFY
#else
#define assert(_x) (_x)
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
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
#include "video_dualdisplay.h"
#endif
#include "common.h"
#include "image.h"
#include "vec3.h"
#include "glext.h"
#include "config.h"
#include "gui.h"
#include "hmd.h"
#include "log.h"
#if ENABLE_MOTIONBLUR!=0
#include "fbo.h"
#endif
#if __cplusplus
}
#endif

#if !__cplusplus
extern const char TITLE[];
extern const char ICON [];
#endif

int opt_touch;
int video_has_touch;

struct video video;

/*---------------------------------------------------------------------------*/

/* Normally...... show the system cursor and hide the virtual cursor.        */
/* In HMD mode... show the virtual cursor and hide the system cursor.        */

#if __cplusplus
extern "C"
#endif
void video_show_cursor(void)
{
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__SWITCH__)
    if (opt_touch)
    {
        gui_set_cursor(0);
        return;
    }

    int cursor_visible = 0;

#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    if (current_platform == PLATFORM_PC)
#endif
    {
#ifdef SWITCHBALL_GUI
        gui_set_cursor(1);
        cursor_visible = SDL_DISABLE;
#else
        if (hmd_stat())
        {
            gui_set_cursor(1);
            cursor_visible = SDL_DISABLE;
        }
        else
        {
            gui_set_cursor(0);
            cursor_visible = SDL_ENABLE;
        }
#endif
    }
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
    else
    {
        /* You won't be able to use the cursor, while using the
         * Nintendo Switch, PS4 or Xbox */
        gui_set_cursor(0);
        cursor_visible = SDL_DISABLE;
    }
#endif

    SDL_ShowCursor(cursor_visible);
#elif defined(__WII__) || defined(__WIIU__)
    gui_set_cursor(1);
#endif
}

/* When the cursor is to be hidden, make sure neither the virtual cursor     */
/* nor the system cursor is visible.                                         */

#if __cplusplus
extern "C"
#endif
void video_hide_cursor(void)
{
    gui_set_cursor(0);

    if (opt_touch) return;

#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
    SDL_ShowCursor(SDL_DISABLE);
#endif
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
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
        image_snap(snapshot_path);
#endif
        snapshot_path[0] = 0;
    }
}

#if __cplusplus
extern "C"
#endif
void video_snap(const char *path)
{
    snapshot_prep(path);
}

/*---------------------------------------------------------------------------*/

#if !defined(__WII__)
static SDL_Window    *window;
static SDL_GLContext  context;
#endif

#if !_MSC_VER && !defined(__APPLE__) && !defined(__WII__)
static void set_window_icon(const char *filename)
{
    if (!window) return;

    SDL_Surface *icon;

    if ((icon = load_surface(filename)))
    {
        SDL_SetWindowIcon(window, icon);

        free(icon->pixels);
        icon->pixels = NULL;

        SDL_FreeSurface(icon);
    }
}
#endif

/*
 * Enter/exit fullscreen mode.
 */
#if __cplusplus
extern "C"
#endif
int video_fullscreen(int f)
{
#if !defined(__WII__)
    int code = SDL_SetWindowFullscreen(window,
                                       f ? SDL_WINDOW_FULLSCREEN_DESKTOP :
                                           0);

    if (code == 0)
        config_set_d(CONFIG_FULLSCREEN, f ? 1 : 0);
    else
        log_errorf("Failure to %s fullscreen (%s)\n", f ? "enter" : "exit",
                   GAMEDBG_GETSTRERROR_CHOICES_SDL);

    return (code == 0);
#else
    return video_mode(1, config_get_d(CONFIG_WIDTH), config_get_d(CONFIG_HEIGHT));
#endif
}

/*
 * Handle a window resize event.
 */
#if __cplusplus
extern "C"
#endif
void video_resize(int window_w, int window_h)
{
#if !defined(__WII__)
    if (window)
    {
        /* Update window size (for mouse events). */

        video.window_w = window_w;
        video.window_h = window_h;

        /* User experience thing: don't save fullscreen size to config. */

        if (!config_get_d(CONFIG_FULLSCREEN))
        {
            config_set_d(CONFIG_WIDTH,  video.window_w);
            config_set_d(CONFIG_HEIGHT, video.window_h);
        }

        /* Update drawable size (for rendering). */

        int wszW, wszH;

        SDL_GetWindowSize(window, &wszW, &wszH);
        SDL_GetWindowSizeInPixels(window, &video.device_w, &video.device_h);

        video.scale_w = floorf((float) wszW / (float) video.device_w);
        video.scale_h = floorf((float) wszH / (float) video.device_h);

        video.device_scale = (float) video.device_h / (float) video.window_h;

        /* Update viewport. */

        glViewport(0, 0,
                   video.device_w * video.scale_w,
                   video.device_h * video.scale_h);

        video.aspect_ratio = (float) video.device_w / (float) video.window_h;
    }
#else
    video_mode(1, window_w, window_h);
#endif
}

#if __cplusplus
extern "C"
#endif
void video_set_window_size(int w, int h)
{
    /*
     * On Emscripten, SDL_SetWindowSize does all of these:
     *
     *   1) updates SDL's cached window->w and window->h values
     *   2) updates canvas.width and canvas.height (drawing buffer size)
     *   3) triggers a SDL_WINDOWEVENT_SIZE_CHANGED event, which updates our
            viewport/UI.
     */

    /*
     * BTW, for this to work with element.requestFullscreen(),
     * a change needs to be applied to the SDL2 Emscripten port:
     * https://github.com/emscripten-ports/SDL2/issues/138
     */

#if defined(__WII__)
    video_mode(1, w, h);
#else
    SDL_SetWindowSize(window, w, h);
#endif
}

#if __cplusplus
extern "C"
#endif
void video_set_display(int dpy)
{
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
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
        log_errorf("Window size exeeds the desktop resolution limit!: Current: %d/%d; Limit: %d/%d\n",
                   video.window_w, video.window_h, ddm.w, ddm.h);
        video_set_window_size(ddm.w, ddm.h);
    }

    if (X - monitor_area_location.x > ddm.w + monitor_area_location.x / 2 ||
        Y - monitor_area_location.y > ddm.h + monitor_area_location.y / 2)
    {
        log_errorf("Window position exeeds the desktop resolution limit!: Current: %d/%d; Limit: %d/%d\n",
                   X, Y, ddm.w, ddm.h);
        X = MAX((ddm.w / 2) - (video.window_w / 2), 0) + monitor_area_location.x;
        Y = MAX((ddm.h / 2) - (video.window_h / 2), 0) + monitor_area_location.y;
    }

    SDL_SetWindowPosition(window, X, Y);
#endif
}

#if __cplusplus
extern "C"
#endif
int video_display(void)
{
#if !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__SWITCH__)
    if (window)
        return SDL_GetWindowDisplayIndex(window);
    else
        return -1;
#else
    return 0;
#endif
}

#if __cplusplus
extern "C"
#endif
int video_init(void)
{
#if defined(__WII__)
    wiigl_create_context();
    video_mode(1,
               config_get_d(CONFIG_WIDTH),
               config_get_d(CONFIG_HEIGHT));
#else
    if (!video_mode(config_get_d(CONFIG_FULLSCREEN),
                    config_get_d(CONFIG_WIDTH),
                    config_get_d(CONFIG_HEIGHT)))
    {
        log_errorf("Failure to create window (%s)\n",
                   GAMEDBG_GETSTRERROR_CHOICES_SDL);
        return 0;
    }
#endif

    return 1;
}

#if __cplusplus
extern "C"
#endif
void video_quit(void)
{
    hmd_free();

#if !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__SWITCH__)
    if (context)
    {
#ifdef __EMSCRIPTEN__
        //close_gl4es();
#endif

        SDL_GL_DeleteContext(context);
        context = NULL;
    }

//#if NB_STEAM_API==0 && !defined(__EMSCRIPTEN__)
    if (window)
    {
        SDL_DestroyWindow(window);
        window = NULL;
    }
//#endif
#endif
}

#if __cplusplus
extern "C"
#endif
int video_mode(int f, int w, int h)
{
#if defined(__WII__)
    video.window_w     = video.device_w = w;
    video.window_h     = video.device_h = h;
    video.device_scale = (float) video.device_h / (float) video.window_h;

    glext_init();

    glViewport(0, 0, video.device_w, video.device_h);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glEnable(GL_NORMALIZE);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glEnable(GL_BLEND);

    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,
                  GL_SEPARATE_SPECULAR_COLOR);

    glPixelStorei(GL_PACK_ALIGNMENT,   1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthFunc(GL_LEQUAL);

    video_show_cursor();
    return 1;
#else
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

#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
    stencil = 1;
#endif

video_mode_reconf:

    stereo   = config_get_d(CONFIG_STEREO)      ? 1 : 0;
    stencil  = config_get_d(CONFIG_REFLECTION)  ? 1 : 0;
    buffers  = config_get_d(CONFIG_MULTISAMPLE) ? 1 : 0;
    samples  = config_get_d(CONFIG_MULTISAMPLE);
    texture  = config_get_d(CONFIG_TEXTURES);
    vsync    = config_get_d(CONFIG_VSYNC)       ? 1 : 0;
    hmd      = config_get_d(CONFIG_HMD)         ? 1 : 0;
    highdpi  = config_get_d(CONFIG_HIGHDPI)     ? 1 : 0;

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

    hmd_free();

    video_quit();

#if ENABLE_OPENGLES
    if (init_gles) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,  SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    }
    else
    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,  0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    }
#endif

#ifndef ENABLE_HMD
    /* If the HMD is not ready, use these. */
    config_set_d(CONFIG_HMD, 0);
    hmd = 0;
#endif

    SDL_GL_SetAttribute(SDL_GL_STEREO,             stereo);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,       stencil);

#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, buffers);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, samples);
#else
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
#endif

    /*
     * Optional 16-bit double buffer with 16-bit depth buffer.
     *
     * Default RGB size: 2 - Either 5 (16-bit) or 8 (32-bit)
     * Default depth size: 8 - Either 8 or 16
     */

    int rgb_size_fixed   = 2;
    int depth_size_fixed = 8;

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   rgb_size_fixed);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, rgb_size_fixed);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  rgb_size_fixed);

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,   depth_size_fixed);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    /* Try to set the currently specified mode. */

    log_printf("Creating a window (%dx%d, %s)\n",
               w, h, (f ? "fullscreen" : "windowed"));

#if NB_STEAM_API==1 && !defined(__EMSCRIPTEN__)
    if (!window) {
#endif
#if __cplusplus
        try {
#endif
#ifndef __EMSCRIPTEN__
        if (w && h && TITLE)
#endif
        {
            window = SDL_CreateWindow(TITLE, X, Y, MAX(w, 320), MAX(h, 240),
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
            SDL_MaximizeWindow(window);
#endif
#if __cplusplus
        } catch (const std::exception& xO) {
            log_errorf("Failure to create window!: Exception caught! (%s)\n", xO.what());
            return 0;
        } catch (const char *xS) {
            log_errorf("Failure to create window!: Exception caught! (%s)\n", xS);
            return 0;
        } catch (...) {
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

    if (window)
    {
#ifndef __EMSCRIPTEN__
#if _WIN32
        SDL_SetWindowMinimumSize(window, 800, 600);
#else
        SDL_SetWindowMinimumSize(window, 320, 240);
#endif
#endif

        if ((context = SDL_GL_CreateContext(window)))
        {
#ifdef __EMSCRIPTEN__
            /* Weird hack to force gl4es to get MAX_TEXTURE_SIZE from WebGL. */
            extern void *emscripten_GetProcAddress(const char *name);
            set_getprocaddress(emscripten_GetProcAddress);

            initialize_gl4es();
#endif

#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
            /*
             * Check whether the sampling and buffer values are
             * NOT in negative values in the multisampling settings.
             */

            if (buffers < 0)
            {
#ifdef __EMSCRIPTEN__
                //close_gl4es();
#endif
                log_errorf("Buffers cannot be negative!\n");
                SDL_GL_DeleteContext(context);
                context = NULL;
            }

            if (samples < 0)
            {
#ifdef __EMSCRIPTEN__
                //close_gl4es();
#endif
                log_errorf("Samples cannot be negative!\n");
                SDL_GL_DeleteContext(context);
                context = NULL;
            }

            int buf, smp;

            SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &buf);
            SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &smp);

            if (buf < buffers || smp < samples)
            {
#ifdef __EMSCRIPTEN__
                //close_gl4es();
#endif
                log_errorf("GL context does not meet minimum specifications!\n");
                SDL_GL_DeleteContext(context);
                context = NULL;
            }
#endif
        }
    }

    if (window && context)
    {
#if !_MSC_VER && !defined(__APPLE__)
        set_window_icon(ICON);
#endif

        log_printf("Created a window (%u, %dx%d, %s)\n",
                   SDL_GetWindowID(window),
                   w, h,
                   (f ? "fullscreen" : "windowed"));

        config_set_d(CONFIG_DISPLAY,    video_display());
        config_set_d(CONFIG_FULLSCREEN, f);

        if (SDL_GL_SetSwapInterval(vsync) != 0)
            config_set_d(CONFIG_VSYNC, 0);

        if (!glext_init())
            goto video_mode_failinit_window_context;

        video_resize(w, h);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

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

#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
        /* If GL supports multisample, and SDL got a multisample buffer... */

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
#endif

#if ENABLE_HMD
        /* Set up HMD display if requested. */

        if (hmd)
            hmd_init();
#endif

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
        config_set_d(CONFIG_WIDTH,   video.window_w);
        config_set_d(CONFIG_HEIGHT,  video.window_h);
        video.aspect_ratio = (float) video.window_w / video.window_h;

        config_save();
        return 1;
    }

video_mode_failinit_window_context:

#if NB_STEAM_API==0 && !defined(__EMSCRIPTEN__)
#if ENABLE_HMD
    /* If the mode failed, try it without stereo. */

    if (stereo)
    {
        config_set_d(CONFIG_STEREO, 0);
        goto video_mode_reconf;
    }
#endif

#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
    /* If the mode failed, try decreasing the level of multisampling. */

    if (buffers)
    {
        config_set_d(CONFIG_MULTISAMPLE, samples / 2);
        goto video_mode_reconf;
    }
#endif

    /* If the mode failed, try decreasing the level of textures. */

    if (texture != 2)
    {
        config_set_d(CONFIG_TEXTURES, 2);
        goto video_mode_reconf;
    }

    /* If that mode failed, try it without reflections. */

    if (stencil)
    {
        config_set_d(CONFIG_REFLECTION, 0);
        goto video_mode_reconf;
    }

    /* If that mode failed, try it without v-sync. */

    if (vsync)
    {
        config_set_d(CONFIG_VSYNC, 0);
        goto video_mode_reconf;
    }

    /* If that mode failed, try it without background. */

    if (config_get_d(CONFIG_BACKGROUND) == 1)
    {
        config_set_d(CONFIG_BACKGROUND, 0);
        goto video_mode_reconf;
    }

    /* If that mode failed, try it without shadow. */

    if (config_get_d(CONFIG_SHADOW) == 1)
    {
        config_set_d(CONFIG_SHADOW, 0);
        goto video_mode_reconf;
    }

#if ENABLE_OPENGLES
    /* If that mode failed, try it without GLES. */

    if (init_gles)
    {
        init_gles = 0;
        goto video_mode_reconf;
    }
#endif

#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
    /* If that mode failed, get the soloution. */

    if (config_get_d(CONFIG_MULTISAMPLE) == 0 && window)
    {
        int solution_buf, solution_smp;
        if (SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &solution_buf) != 0 ||
            SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &solution_smp) != 0)
            return 0;

        if (solution_buf < solution_smp)
            config_set_d(CONFIG_MULTISAMPLE, solution_smp);
        else if (solution_buf > solution_smp)
            config_set_d(CONFIG_MULTISAMPLE, solution_buf);
        else
            config_set_d(CONFIG_MULTISAMPLE, solution_smp);

#ifndef NDEBUG
        assert(solution_buf > -1 && solution_smp > -1);
#endif

        goto video_mode_reconf;
    }
#endif

    /* If THAT mode failed, punt. */
#else
#if defined(ENABLE_MULTISAMPLE_SOLUTION)
    /* Get the soloution first. */

    if (config_get_d(CONFIG_MULTISAMPLE) == 0 && window)
    {
        int solution_buf, solution_smp;
        if (SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &solution_buf) != 0 ||
            SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &solution_smp) != 0)
            return 0;

        if (solution_buf < solution_smp)
            config_set_d(CONFIG_MULTISAMPLE, solution_smp);
        else if (solution_buf > solution_smp)
            config_set_d(CONFIG_MULTISAMPLE, solution_buf);
        else
            config_set_d(CONFIG_MULTISAMPLE, solution_smp);

#ifndef NDEBUG
        assert(solution_buf > -1 && solution_smp > -1);
#endif

        goto video_mode_reconf;
    }
#endif

    /* For some reasons, this may not work on oldest hardware. */

#endif

    return 0;
#endif
}

#if __cplusplus
extern "C"
#endif
int video_mode_auto_config(int f, int w, int h)
{
#if defined(__WII__)
    video.window_w     = video.device_w = w;
    video.window_h     = video.device_h = h;
    video.device_scale = (float) video.device_h / (float) video.window_h;

    glext_init();

    glViewport(0, 0, video.device_w, video.device_h);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glEnable(GL_NORMALIZE);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glEnable(GL_BLEND);

    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,
                  GL_SEPARATE_SPECULAR_COLOR);

    glPixelStorei(GL_PACK_ALIGNMENT,   1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthFunc(GL_LEQUAL);

    video_show_cursor();
    return 1;
#else
#if ENABLE_OPENGLES
    int init_gles = 1;
#endif

    int stereo   = config_get_d(CONFIG_STEREO)      ? 1 : 0;
    int hmd      = config_get_d(CONFIG_HMD)         ? 1 : 0;

video_mode_auto_config_reconf:
    stereo   = config_get_d(CONFIG_STEREO)      ? 1 : 0;
    hmd      = config_get_d(CONFIG_HMD)         ? 1 : 0;

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

    hmd_free();

    video_quit();

#if ENABLE_OPENGLES
    if (init_gles) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,  SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    }
    else
    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,  0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    }
#endif

    SDL_GL_SetAttribute(SDL_GL_STEREO,             stereo);

    int auto_stencils = 1;
    int stencil_ok    = SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, auto_stencils);

    int auto_samples = 16;

#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
    int smpbuf_ok    = SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, auto_samples ? 1 : 0);
    int smp_ok       = SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, auto_samples);
#else
    int smpbuf_ok    = SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
    int smp_ok       = SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
#endif

    /*
     * Optional 16-bit double buffer with 16-bit depth buffer.
     *
     * TODO: Uncomment, if you want to set the required buffer.
     * Default RGB size: 2 - Either 5 (16-bit) or 8 (32-bit)
     * Default depth size: 8 - Either 8 or 16
     */

    int rgb_size_fixed   = 2;
    int depth_size_fixed = 8;

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   rgb_size_fixed);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, rgb_size_fixed);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  rgb_size_fixed);

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,   depth_size_fixed);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    log_printf("Creating a window (%dx%d, %s (auto configuration))\n",
               w, h, (f ? "fullscreen" : "windowed"));

#if NB_STEAM_API==1 && !defined(__EMSCRIPTEN__)
    if (!window) {
#endif
#if __cplusplus
        try {
#endif
#ifndef __EMSCRIPTEN__
        if (w && h && TITLE)
#endif
        {
            window = SDL_CreateWindow(TITLE, X, Y, MAX(w, 320), MAX(h, 240),
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
            SDL_MaximizeWindow(window);
#endif
#if __cplusplus
        } catch (const std::exception& xO) {
            log_errorf("Failure to create window!: Exception caught! (%s)\n", xO.what());
            return 0;
        } catch (const char *xS) {
            log_errorf("Failure to create window!: Exception caught! (%s)\n", xS);
            return 0;
        } catch (...) {
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

    int got_geforce = 0;

    while (auto_stencils > 0 && got_geforce == 0)
    {
        if (stencil_ok == 0)
        {
            if ((context = SDL_GL_CreateContext(window)))
            {
                int stn;

                if (SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &stn) != 0)
                    return 0;

                if (stn >= auto_stencils)
                {
#ifdef __EMSCRIPTEN__
                    //close_gl4es();
#endif
                    SDL_GL_DeleteContext(context);
                    context = NULL;
                    config_set_d(CONFIG_REFLECTION, auto_stencils);
                    got_geforce = 1;
                }
                else if (auto_stencils != 0)
                {
#ifdef __EMSCRIPTEN__
                    //close_gl4es();
#endif
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
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
                int buf, smp;

                if (SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &buf) != 0 ||
                    SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &smp) != 0)
                    return 0;

                if (buf >= 1 && smp >= auto_samples)
                {
#ifdef __EMSCRIPTEN__
                    //close_gl4es();
#endif
                    SDL_GL_DeleteContext(context);
                    context = NULL;
                    config_set_d(CONFIG_MULTISAMPLE, auto_samples);
                    got_geforce = 1;
                }
                else if (auto_samples != 0)
                {
#ifdef __EMSCRIPTEN__
                    //close_gl4es();
#endif
                    SDL_GL_DeleteContext(context);
                    context = NULL;
                    auto_samples /= 2;
                    if (SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, auto_samples) != 0 &&
                        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, auto_samples ? 1 : 0) != 0)
                        return 0;
                }
                else
                    return 0;
#endif
            }
        }
        else
            return 0;
    }

    if (window)
    {
#ifndef __EMSCRIPTEN__
#if _WIN32
        SDL_SetWindowMinimumSize(window, 800, 600);
#else
        SDL_SetWindowMinimumSize(window, 320, 240);
#endif
#endif

        config_set_d(CONFIG_HIGHDPI, 1);
        if ((context = SDL_GL_CreateContext(window)))
        {
#ifdef __EMSCRIPTEN__
            /* Weird hack to force gl4es to get MAX_TEXTURE_SIZE from WebGL. */
            extern void *emscripten_GetProcAddress(const char *name);
            set_getprocaddress(emscripten_GetProcAddress);

            initialize_gl4es();
#endif

#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
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
#endif
        }
    }

    if (window && context)
    {
#if !_MSC_VER && !defined(__APPLE__)
        set_window_icon(ICON);
#endif

        log_printf("Created a window (%u, %dx%d, %s (auto configuration))\n",
                   SDL_GetWindowID(window),
                   w, h,
                   (f ? "fullscreen" : "windowed"));

        config_set_d(CONFIG_DISPLAY,    video_display());
        config_set_d(CONFIG_FULLSCREEN, f);
        config_set_d(CONFIG_VSYNC,      1);

        if (SDL_GL_SetSwapInterval(1) != 0)
            config_set_d(CONFIG_VSYNC, 0);

        if (!glext_init())
            goto video_mode_auto_config_failinit_window_context;

        video_resize(w, h);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

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

#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
        /* If GL supports multisample, and SDL got a multisample buffer... */

        if (glext_check_ext("ARB_multisample"))
        {
            int buffers;
            SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &buffers);
            if (buffers) glEnable(GL_MULTISAMPLE);
        }
        else
        {
            glDisable(GL_MULTISAMPLE);
            config_set_d(CONFIG_MULTISAMPLE, 0);
        }
#endif

#if ENABLE_HMD
        /* Set up HMD display if requested. */

        if (hmd)
            hmd_init();
#endif

        /* Initialize screen snapshotting. */

        snapshot_init();

        video_show_cursor();

        /* Grab input immediately in HMD mode. */

        if (hmd_stat())
            SDL_SetWindowGrab(window, SDL_TRUE);

        /* Set it back for recommended configurations. */

        config_set_d(CONFIG_TEXTURES,   1);
        config_set_d(CONFIG_SHADOW,     1);
        config_set_d(CONFIG_BACKGROUND, 1);
#ifdef GL_GENERATE_MIPMAP_SGIS
        config_set_d(CONFIG_MIPMAP,     1);
#endif
#if defined(GL_TEXTURE_MAX_ANISOTROPY_EXT) && \
    GL_TEXTURE_MAX_ANISOTROPY_EXT != 0 && GL_TEXTURE_MAX_ANISOTROPY_EXT != -1
        config_set_d(CONFIG_ANISO,      2);
#endif

        /* Recenter the cursor back in full screen mode. */

        if (config_get_d(CONFIG_FULLSCREEN))
            SDL_WarpMouseInWindow(window,
                                  video.window_w / 2,
                                  video.window_h / 2);

        config_set_d(CONFIG_DISPLAY, dpy);
        config_set_d(CONFIG_WIDTH,   video.window_w);
        config_set_d(CONFIG_HEIGHT,  video.window_h);
        video.aspect_ratio = (float) video.window_w / video.window_h;

        config_save();

        return 1;
    }

video_mode_auto_config_failinit_window_context:

#if ENABLE_OPENGLES
    if (init_gles)
    {
        init_gles = 0;
        goto video_mode_auto_config_reconf;
    }
#endif

#if defined(ENABLE_MULTISAMPLE_SOLUTION) && \
    !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
    /* Get the soloution first. */

    if (config_get_d(CONFIG_MULTISAMPLE) == 0 && window)
    {
        int solution_buf, solution_smp;
        if (SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &solution_buf) != 0 ||
            SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &solution_smp) != 0)
            return 0;

        if (solution_buf < solution_smp)
            config_set_d(CONFIG_MULTISAMPLE, solution_smp);
        else if (solution_buf > solution_smp)
            config_set_d(CONFIG_MULTISAMPLE, solution_buf);
        else
            config_set_d(CONFIG_MULTISAMPLE, solution_smp);

#ifndef NDEBUG
        assert(solution_buf > -1 && solution_smp > -1);
#endif

        goto video_mode_auto_config_reconf;
    }
#endif
    /* For some reasons, this may not work on oldest hardware. */

    return 0;
#endif
}

/*---------------------------------------------------------------------------*/

int video_can_swap_window = 1;

static float ms     = 0;
static int   fps    = 0;
static int   last   = 0;
static int   ticks  = 0;
static int   frames = 0;

#if __cplusplus
extern "C"
#endif
int  video_perf(void)
{
    return fps;
}

#if __cplusplus
extern "C"
#endif
void video_swap(void)
{
    if (!video_can_swap_window)
        return;

    video_can_swap_window = 0;

    int dt;

#if ENABLE_MOTIONBLUR!=0
    if (config_get_d(CONFIG_MOTIONBLUR))
        video_motionblur_swap();
#endif

#if defined(__WII__)
    wiigl_swap_buffers();
#else
    if (hmd_stat())
        hmd_swap();

    /* Take a screenshot of the complete back buffer and swap it. */

    snapshot_take();

    SDL_GL_SwapWindow(window);
#endif

    /* Accumulate time passed and frames rendered. */

    dt    = (int) SDL_GetTicks() - last;
    last += dt;

#ifdef FPS_REALTIME
    /* Compute frame time and frames-per-second stats. */

    if (dt > 0 && dt < 100)
    {
        fps = 1 / (0.001f * dt);
        ms  = 0.001f * dt;
    }

#if NB_STEAM_API==0 && !defined(LOG_NO_STATS)
    /* Output statistics if configured. */

    if (config_get_d(CONFIG_STATS) && dt > 0 && dt < 100)
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
        ticks  -= 1000;

#if NB_STEAM_API==0 && !defined(LOG_NO_STATS)
        /* Output statistics if configured. */

        if (config_get_d(CONFIG_STATS) && dt > 0 && dt < 100)
            fprintf(stdout, "%4d %8.4f\n", fps, (double) ms);
#endif
    }
#endif
}

/*---------------------------------------------------------------------------*/

static int grabbed = 0;

#if __cplusplus
extern "C"
#endif
void video_set_grab(int w)
{
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
    if (opt_touch) return;

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
#endif

    video_hide_cursor();

    grabbed = 1;
}

#if __cplusplus
extern "C"
#endif
void video_clr_grab(void)
{
#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
    SDL_SetRelativeMouseMode(SDL_FALSE);

    /* Never release the grab in HMD mode. */

    if (!hmd_stat())
        SDL_SetWindowGrab(window, SDL_FALSE);
#endif

    video_show_cursor();

    grabbed = 0;
}

#if __cplusplus
extern "C"
#endif
int  video_get_grab(void)
{
    return grabbed;
}

/*---------------------------------------------------------------------------*/

#if ENABLE_MOTIONBLUR!=0
static int motionblur_renderable[VIDEO_MOTIONBLUR_MAX_TEXTURE];

static unsigned int motionblur_vbo;
static unsigned int motionblur_ebo;

static int   motionblur_index;
static float motionblur_alpha;

#if __cplusplus
extern "C"
#endif
void video_motionblur_init(void)
{
}

#if __cplusplus
extern "C"
#endif
void video_motionblur_quit(void)
{
}

#if __cplusplus
extern "C"
#endif
void video_motionblur_alpha_set(float a)
{
    motionblur_alpha = a;
}

#if __cplusplus
extern "C"
#endif
float video_motionblur_alpha_get(void)
{
    return motionblur_alpha;
}

#if __cplusplus
extern "C"
#endif
void video_motionblur_prep(void)
{
    if (motionblur_index >= VIDEO_MOTIONBLUR_MAX_TEXTURE)
        return;

    if (motionblur_renderable[motionblur_index] != 0)
        return;

    glViewport(0, 0, video.device_w, video.device_h);

    glClear(GL_DEPTH_BUFFER_BIT |
            (config_get_d(CONFIG_REFLECTION) ? GL_STENCIL_BUFFER_BIT : 0));
}

#if __cplusplus
extern "C"
#endif
void video_motionblur_swap(void)
{
    motionblur_index = 0;
}
#endif

/*---------------------------------------------------------------------------*/

#if __cplusplus
extern "C"
#endif
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

#if __cplusplus
extern "C"
#endif
void video_set_perspective(float fov, float n, float f)
{
    if (hmd_stat())
        hmd_persp(n, f);
    else
    {
        GLfloat r = fov / 2 * V_PI / 180;
        GLfloat s = fsinf(r);
        GLfloat c = fcosf(r) / s;

        GLfloat a = ((GLfloat) video.device_w /
                     (GLfloat) video.device_h);

        glMatrixMode(GL_PROJECTION);
        {
#if defined(__WII__)
            /* TODO: figure out why the matrix isn't working */
            GLfloat fH = ftanf(fov / 360.0 * V_PI) * n;
            GLfloat fW = fH * a;
            glFrustum(-fW, fW, -fH, fH, n, f);
#else
            GLfloat m[16] = {
                c / a, 0.0f,  0.0f,                    0.0f,
                0.0f,  c,     0.0f,                    0.0f,
                0.0f,  0.0f, -(f + n) / (f - n),      -1.0f,
                0.0f,  0.0f, -2.0f * n * f / (f - n),  0.0f
            };

            glLoadMatrixf(m);
#endif
        }

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    }
}

#if __cplusplus
extern "C"
#endif
void video_set_ortho(void)
{
    if (hmd_stat())
        hmd_ortho();
    else
    {
        GLfloat w = (GLfloat) video.device_w;
        GLfloat h = (GLfloat) video.device_h;

        glMatrixMode(GL_PROJECTION);

        glLoadIdentity();
        glOrtho_(0.0, w, 0.0, h, -1.0, +1.0);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    }
}

#if __cplusplus
extern "C"
#endif
void video_set_current(void)
{
#if !defined(__WII__)
    if (!window || !context) return;
#endif

#if ENABLE_DUALDISPLAY==1
    //SDL_GL_MakeCurrent(window, context);
#endif
}

#if __cplusplus
extern "C"
#endif
void video_clear(void)
{
    glViewport(0, 0, video.device_w, video.device_h);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
            (config_get_d(CONFIG_REFLECTION) ? GL_STENCIL_BUFFER_BIT : 0));
}

/*---------------------------------------------------------------------------*/
