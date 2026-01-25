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

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
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

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "glext.h"
#include "log.h"

struct gl_info gli;

/*---------------------------------------------------------------------------*/

#if !defined(__WII__)
#if !ENABLE_OPENGLES && !defined(__EMSCRIPTEN__)

PFNGLCLIENTACTIVETEXTURE_PROC    glClientActiveTexture_;
PFNGLACTIVETEXTURE_PROC          glActiveTexture_;

PFNGLGENBUFFERS_PROC             glGenBuffers_;
PFNGLBINDBUFFER_PROC             glBindBuffer_;
PFNGLBUFFERDATA_PROC             glBufferData_;
PFNGLBUFFERSUBDATA_PROC          glBufferSubData_;
PFNGLDELETEBUFFERS_PROC          glDeleteBuffers_;
PFNGLISBUFFER_PROC               glIsBuffer_;

PFNGLPOINTPARAMETERF_PROC        glPointParameterf_;
PFNGLPOINTPARAMETERFV_PROC       glPointParameterfv_;

PFNGLGETSHADERIV_PROC            glGetShaderiv_;
PFNGLGETSHADERINFOLOG_PROC       glGetShaderInfoLog_;
PFNGLGETPROGRAMIV_PROC           glGetProgramiv_;
PFNGLGETPROGRAMINFOLOG_PROC      glGetProgramInfoLog_;
PFNGLCREATESHADER_PROC           glCreateShader_;
PFNGLCREATEPROGRAM_PROC          glCreateProgram_;
PFNGLSHADERSOURCE_PROC           glShaderSource_;
PFNGLCOMPILESHADER_PROC          glCompileShader_;
PFNGLDELETESHADER_PROC           glDeleteShader_;
PFNGLDELETEPROGRAM_PROC          glDeleteProgram_;
PFNGLATTACHSHADER_PROC           glAttachShader_;
PFNGLLINKPROGRAM_PROC            glLinkProgram_;
PFNGLUSEPROGRAM_PROC             glUseProgram_;
PFNGLGETUNIFORMLOCATION_PROC     glGetUniformLocation_;
PFNGLUNIFORM1F_PROC              glUniform1f_;
PFNGLUNIFORM2F_PROC              glUniform2f_;
PFNGLUNIFORM3F_PROC              glUniform3f_;
PFNGLUNIFORM4F_PROC              glUniform4f_;

PFNGLBINDFRAMEBUFFER_PROC        glBindFramebuffer_;
PFNGLDELETEFRAMEBUFFERS_PROC     glDeleteFramebuffers_;
PFNGLGENFRAMEBUFFERS_PROC        glGenFramebuffers_;
PFNGLFRAMEBUFFERTEXTURE2D_PROC   glFramebufferTexture2D_;
PFNGLCHECKFRAMEBUFFERSTATUS_PROC glCheckFramebufferStatus_;

PFNGLSTRINGMARKERGREMEDY_PROC    glStringMarkerGREMEDY_;

#if _WIN32 || _WIN64
#if ENABLE_GL_NV
PFNGLVIEWPORTPOSITIONWSCALENV_PROC glViewportPositionWScaleNV_;

PFNGLGENOCCLUSIONQUERIESNV_PROC    glGenOcclusionQueriesNV_;
PFNGLDELETEOCCLUSIONQUERIESNV_PROC glDeleteOcclusionQueriesNV_;
PFNGLISOCCLUSIONQUERYNV_PROC       glIsOcclusionQueryNV_;
PFNGLBEGINOCCLUSIONQUERYNV_PROC    glBeginOcclusionQueryNV_;
PFNGLENDOCCLUSIONQUERYNV_PROC      glEndOcclusionQueryNV_;
PFNGLGETOCCLUSIONQUERYIVNV_PROC    glGetOcclusionQueryivNV_;
PFNGLGETOCCLUSIONQUERYUIVNV_PROC   glGetOcclusionQueryuivNV_;

PFNGLFLUSHVERTEXARRAYRANGENV_PROC  glFlushVertexArrayRangeNV_;
PFNGLVERTEXARRAYRANGENV_PROC       glVertexArrayRangeNV_;

PFNGLCREATESTATESNV_PROC                 glCreateStatesNV_;
PFNGLDELETESTATESNV_PROC                 glDeleteStatesNV_;
PFNGLISSTATENV_PROC                      glIsStateNV_;
PFNGLSTATECAPTURENV_PROC                 glStateCaptureNV_;
PFNGLGETCOMMANDHEADERNV_PROC             glGetCommandHeaderNV_;
PFNGLGETSTAGEINDEXNV_PROC                glGetStageIndexNV_;
PFNGLDRAWCOMMANDSNV_PROC                 glDrawCommandsNV_;
PFNGLDRAWCOMMANDSADDRESSNV_PROC          glDrawCommandsAddressNV_;
PFNGLDRAWCOMMANDSSTATESNV_PROC           glDrawCommandsStatesNV_;
PFNGLDRAWCOMMANDSSTATESADDRESSNV_PROC    glDrawCommandsStatesAddressNV_;
PFNGLCREATECOMMANDLISTSNV_PROC           glCreateCommandListsNV_;
PFNGLDELETECOMMANDLISTSNV_PROC           glDeleteCommandListsNV_;
PFNGLISCOMMANDLISTNV_PROC                glIsCommandListNV_;
PFNGLLISTDRAWCOMMANDSSTATESCLIENTNV_PROC glListDrawCommandsStatesClientNV_;
PFNGLCOMMANDLISTSEGMENTSNV_PROC          glCommandListSegmentsNV_;
PFNGLCOMPILECOMMANDLISTNV_PROC           glCompileCommandListNV_;
PFNGLCALLCOMMANDLISTNV_PROC              glCallCommandListNV_;
#endif
#endif

#endif
#endif

/*---------------------------------------------------------------------------*/

#undef glMatrixMode
#undef glPushMatrix
#undef glPopMatrix

static unsigned int glext_matrix_mode;

static int glext_curr_depth_modelview;
static int glext_curr_depth_projection;

/*---------------------------------------------------------------------------*/

#define str_starts_with(s, h) (strncmp((s), (h), strlen(h)) == 0)

/*---------------------------------------------------------------------------*/

int glext_check_renderer(const char *renderer)
{
    return str_starts_with((const char *) glGetString(GL_RENDERER), renderer);
}

int glext_check_ext(const char *needle)
{
    const GLubyte *haystack, *c;

    /* Search for the given string in the OpenGL extension strings. */

    for (haystack = glGetString(GL_EXTENSIONS); haystack && *haystack; haystack++)
    {
        for (c = (const GLubyte *) needle; *c && *haystack; c++, haystack++)
            if (*c != *haystack)
                break;

        if ((*c == 0) && (*haystack == ' ' || *haystack == '\0'))
            return 1;
    }

    return 0;
}

/*---------------------------------------------------------------------------*/

#if !ENABLE_OPENGLES && !defined(__EMSCRIPTEN__) && !defined(__WII__)
#if _DEBUG
/*
 * HACK: WGL always makes sense to debug for the best experiences.
 * Useful when debugging game development for Visual Studio for Windows,
 * when making sure that will be bugfixed. - Ersohn Styne
 */

int glext_fail(const char *title, const char *message);

static int glext_assert_dbg(const char *ext)
{
    int have_ext = glext_check_ext(ext);

    if (!have_ext)
    {
        char outStr[32];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        sprintf_s(outStr, 32,
#else
        sprintf(outStr,
#endif
                "Missing required OpenGL extension (%s)", ext);
        have_ext = glext_fail("Missing extensions!", outStr);
        SDL_TriggerBreakpoint();
    }

    return have_ext;
}

#define glext_assert(ext) glext_assert_dbg(ext)
#else
static int glext_assert(const char *ext)
{
    if (!glext_check_ext(ext))
    {
        log_errorf("Missing required OpenGL extension (%s)\n", ext);
        return 0;
    }
    return 1;
}
#endif

static int glext_count(void)
{
    int n = 0;

#if !ENABLE_OPENGLES && !defined(__EMSCRIPTEN__)
    const GLubyte *haystack, *c;

    for (haystack = glGetString(GL_EXTENSIONS); *haystack; haystack++)
    {
        for (c = "GL_"; *c && *haystack; c++, haystack++)
        {
            if (*c == *haystack) n++;
            else break;
        }
    }
#endif

    return n;
}
#endif

/*---------------------------------------------------------------------------*/

#if _DEBUG
#define SDL_GL_GFPA(fun, str) do {                             \
    ptr = SDL_GL_GetProcAddress(str);                          \
    while (!ptr) {                                             \
        char outStr[32];                                       \
        sprintf(outStr, "Missing OpenGL function: (%s)", str); \
        glext_fail("Missing function!", outStr);               \
        SDL_TriggerBreakpoint();                               \
        return 0;                                              \
    } memcpy(&fun, &ptr, sizeof (void *));                     \
} while(0)
#else
#define SDL_GL_GFPA(fun, str) do {                       \
    ptr = SDL_GL_GetProcAddress(str);                    \
    if (!ptr) {                                          \
        log_errorf("Missing OpenGL function: %s\n", str);\
        return 0;                                        \
    } memcpy(&fun, &ptr, sizeof (void *));               \
} while(0)
#endif

/*---------------------------------------------------------------------------*/

#ifdef __EMSCRIPTEN__
#define GLEXT_COUNT_LIMIT 500
#else
#define GLEXT_COUNT_LIMIT 1500
#endif

#if _DEBUG
#define GLEXT_COUNT_LIMIT_WITH_BREAKPT
#endif

static void log_opengl(void)
{
    log_printf("GL vendor: %s\n"
               "GL renderer: %s\n"
               "GL version: %s\n"
               "GL extensions: %s\n",
                glGetString(GL_VENDOR),
                glGetString(GL_RENDERER),
                glGetString(GL_VERSION),
                glGetString(GL_EXTENSIONS));
}

int glext_fail(const char *title, const char *message)
{
#if _WIN32
    /* Let's do Windows message box stuff, because it's already in there. */

    MessageBoxA(0, message, title, MB_ICONERROR);
#elif defined(__WII__)
    printf("%s: %s\n", title, message);
#else
    if (SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, message, NULL) != 0)
    {
#if defined(__linux__)
        char msgbox_cmd[256];
        SAFECPY(msgbox_cmd, "zenity --error --title=\"");
        SAFECAT(msgbox_cmd, title);
        SAFECAT(msgbox_cmd, "\" --text=\"");
        SAFECAT(msgbox_cmd, message);
        SAFECAT(msgbox_cmd, "\" --ok-label=\"OK\"");

        system(msgbox_cmd);
#endif
    }
#endif
    return 0;
}

int glext_init(void)
{
    void *ptr = 0;

#if !ENABLE_OPENGLES && !defined(__EMSCRIPTEN__) && !defined(__WII__)
    if (glext_count() >= GLEXT_COUNT_LIMIT)
    {
#ifdef GLEXT_COUNT_LIMIT_WITH_BREAKPT
        glext_fail("Extensions overloaded!", "Too many GL extensions on this PC!");
        SDL_TriggerBreakpoint();
        return 0;
#else
        log_printf("Too many GL extensions on this PC!\n");
#endif
    }
#endif

    memset(&gli, 0, sizeof (struct gl_info));

    /* Common init. */
    
    glGetIntegerv(GL_MAX_MODELVIEW_STACK_DEPTH,  &glext_max_depth_modelview);
    glGetIntegerv(GL_MAX_PROJECTION_STACK_DEPTH, &glext_max_depth_projection);

    if (glext_max_depth_modelview  < 6 ||
        glext_max_depth_projection < 2)
        return 0;

    glGetIntegerv(GL_MAX_TEXTURE_SIZE,  &gli.max_texture_size);
    glGetIntegerv(GL_MAX_TEXTURE_UNITS, &gli.max_texture_units);

    if (glext_check_ext("GL_EXT_texture_filter_anisotropic"))
        gli.texture_filter_anisotropic = 1;

#if !defined(__WII__)
    /* Desktop init. */

#if !ENABLE_OPENGLES && !defined(__EMSCRIPTEN__)

    if (glext_assert("ARB_multitexture"))
    {
        SDL_GL_GFPA(glClientActiveTexture_, "glClientActiveTextureARB");
        SDL_GL_GFPA(glActiveTexture_,       "glActiveTextureARB");
    }
    else return 0;

    if (glext_assert("ARB_vertex_buffer_object"))
    {
        SDL_GL_GFPA(glGenBuffers_,          "glGenBuffersARB");
        SDL_GL_GFPA(glBindBuffer_,          "glBindBufferARB");
        SDL_GL_GFPA(glBufferData_,          "glBufferDataARB");
        SDL_GL_GFPA(glBufferSubData_,       "glBufferSubDataARB");
        SDL_GL_GFPA(glDeleteBuffers_,       "glDeleteBuffersARB");
        SDL_GL_GFPA(glIsBuffer_,            "glIsBufferARB");
    }
    else return 0;

    if (glext_assert("ARB_point_parameters"))
    {
        SDL_GL_GFPA(glPointParameterf_,    "glPointParameterfARB");
        SDL_GL_GFPA(glPointParameterfv_,   "glPointParameterfvARB");
    }
    else return 0;

    if (glext_check_ext("ARB_shader_objects"))
    {
        SDL_GL_GFPA(glGetShaderiv_,        "glGetShaderiv");
        SDL_GL_GFPA(glGetShaderInfoLog_,   "glGetShaderInfoLog");
        SDL_GL_GFPA(glGetProgramiv_,       "glGetProgramiv");
        SDL_GL_GFPA(glGetProgramInfoLog_,  "glGetProgramInfoLog");
        SDL_GL_GFPA(glCreateShader_,       "glCreateShader");
        SDL_GL_GFPA(glCreateProgram_,      "glCreateProgram");
        SDL_GL_GFPA(glShaderSource_,       "glShaderSource");
        SDL_GL_GFPA(glCompileShader_,      "glCompileShader");
        SDL_GL_GFPA(glDeleteShader_,       "glDeleteShader");
        SDL_GL_GFPA(glDeleteProgram_,      "glDeleteProgram");
        SDL_GL_GFPA(glAttachShader_,       "glAttachShader");
        SDL_GL_GFPA(glLinkProgram_,        "glLinkProgram");
        SDL_GL_GFPA(glUseProgram_,         "glUseProgram");
        SDL_GL_GFPA(glGetUniformLocation_, "glGetUniformLocation");
        SDL_GL_GFPA(glUniform1f_,          "glUniform1f");
        SDL_GL_GFPA(glUniform2f_,          "glUniform2f");
        SDL_GL_GFPA(glUniform3f_,          "glUniform3f");
        SDL_GL_GFPA(glUniform4f_,          "glUniform4f");

        gli.shader_objects = 1;
    }

    if (glext_check_ext("ARB_framebuffer_object"))
    {
        SDL_GL_GFPA(glBindFramebuffer_,        "glBindFramebuffer");
        SDL_GL_GFPA(glDeleteFramebuffers_,     "glDeleteFramebuffers");
        SDL_GL_GFPA(glGenFramebuffers_,        "glGenFramebuffers");
        SDL_GL_GFPA(glFramebufferTexture2D_,   "glFramebufferTexture2D");
        SDL_GL_GFPA(glCheckFramebufferStatus_, "glCheckFramebufferStatus");

        gli.framebuffer_object = 1;
    }

    if (glext_check_ext("GREMEDY_string_marker"))
    {
        SDL_GL_GFPA(glStringMarkerGREMEDY_, "glStringMarkerGREMEDY");

        gli.string_marker = 1;
    }

    /* NVIDIA init. */

#if ENABLE_GL_NV
    if (glext_assert("GL_NV_vertex_array_range"))
    {
        SDL_GL_GFPA(glFlushVertexArrayRangeNV_, "glFlushVertexArrayRangeNV");
        SDL_GL_GFPA(glVertexArrayRangeNV_,      "glVertexArrayRangeNV");
    }
    else return 0;

    if (glext_assert("GL_NV_command_list"))
    {
        SDL_GL_GFPA(glCreateStatesNV_,                 "glCreateStatesNV");
        SDL_GL_GFPA(glDeleteStatesNV_,                 "glDeleteStatesNV");
        SDL_GL_GFPA(glIsStateNV_,                      "glIsStateNV");
        SDL_GL_GFPA(glStateCaptureNV_,                 "glStateCaptureNV");
        SDL_GL_GFPA(glGetCommandHeaderNV_,             "glGetCommandHeaderNV");
        SDL_GL_GFPA(glGetStageIndexNV_,                "glGetStageIndexNV");
        SDL_GL_GFPA(glDrawCommandsNV_,                 "glDrawCommandsNV");
        SDL_GL_GFPA(glDrawCommandsAddressNV_,          "glDrawCommandsAddressNV");
        SDL_GL_GFPA(glDrawCommandsStatesNV_,           "glDrawCommandsStatesNV");
        SDL_GL_GFPA(glDrawCommandsStatesAddressNV_,    "glDrawCommandsStatesAddressNV");
        SDL_GL_GFPA(glCreateCommandListsNV_,           "glCreateCommandListsNV");
        SDL_GL_GFPA(glDeleteCommandListsNV_,           "glDeleteCommandListsNV");
        SDL_GL_GFPA(glIsCommandListNV_,                "glIsCommandListNV");
        SDL_GL_GFPA(glListDrawCommandsStatesClientNV_, "glListDrawCommandsStatesClientNV");
        SDL_GL_GFPA(glCommandListSegmentsNV_,          "glCommandListSegmentsNV");
        SDL_GL_GFPA(glCompileCommandListNV_,           "glCompileCommandListNV");
        SDL_GL_GFPA(glCallCommandListNV_,              "glCallCommandListNV");
    }
    else return 0;

    if (glext_check_ext("GL_NV_clip_space_w_scaling"))
    {
        SDL_GL_GFPA(glViewportPositionWScaleNV_, "glViewportPositionWScaleNV");
    }

    if (glext_check_ext("GL_NV_occlusion_query"))
    {
        SDL_GL_GFPA(glGenOcclusionQueriesNV_,    "glGenOcclusionQueriesNV");
        SDL_GL_GFPA(glDeleteOcclusionQueriesNV_, "glDeleteOcclusionQueriesNV");
        SDL_GL_GFPA(glIsOcclusionQueryNV_,       "glIsOcclusionQueryNV");
        SDL_GL_GFPA(glBeginOcclusionQueryNV_,    "glBeginOcclusionQueryNV");
        SDL_GL_GFPA(glEndOcclusionQueryNV_,      "glEndOcclusionQueryNV");
        SDL_GL_GFPA(glGetOcclusionQueryivNV_,    "glGetOcclusionQueryivNV");
        SDL_GL_GFPA(glGetOcclusionQueryuivNV_,   "glGetOcclusionQueryuivNV");
    }

#endif

#endif
#endif

    log_opengl();

    return 1;
}

int glext_get_recommended(void)
{
#ifndef __EMSCRIPTEN__
    return glext_check_renderer("NVIDIA") || glext_check_renderer("AMD") ||
           glext_check_renderer("GIGABYTE") || glext_check_renderer("Radeon");
#else
    const int r = EM_ASM_INT({
        try {
            var main_canvas = document.getElementById("canvas");
            var main_gl = main_canvas.getContext("webgl", { powerPreference: "high-performance" }); || main_canvas.getContext("experimental-webgl", { powerPreference: "high-performance" });

            if (!main_gl) return 0;

            var main_debug_info = main_gl.getExtension('WEBGL_debug_renderer_info');

            if (!main_debug_info) return 0;

            var main_renderer = main_gl.getParameter(main_debug_info.UNMASKED_RENDERER_WEBGL).toLowerCase();
            var main_software_keywords = ["software","swiftshader","llvmpipe","microsoft basic render driver"];
            return !main_software_keywords.some(function(keyword) { return main_renderer.includes("keyword") });
        } catch (e) {}

        return 0;
    });

    return r;
#endif
}

/*---------------------------------------------------------------------------*/

void glClipPlane4f_(GLenum p, GLfloat a, GLfloat b, GLfloat c, GLfloat d)
{
    GLdouble v[4] = { a, b, c, d };

    glClipPlane(p, v);
}

void glBindTexture_(GLenum target, GLuint texture)
{
    //if (gli.wireframe) texture = 0u;

    glBindTexture(target, texture);
}

void glToggleWireframe_(void)
{
    glSetWireframe_(!gli.wireframe);
}

void glSetWireframe_(int enabled)
{
    gli.wireframe = enabled;

#if !ENABLE_OPENGLES
    if (enabled)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
}

/*---------------------------------------------------------------------------*/

#if !ENABLE_OPENGL_ES && !defined(__EMSCRIPTEN__)

#undef glMatrixMode
#undef glPushMatrix
#undef glPopMatrix

void glMatrixMode_(unsigned int mode)
{
    if (mode != glext_matrix_mode &&
        (mode == GL_MODELVIEW  ||
         mode == GL_PROJECTION ||
         mode == GL_TEXTURE))
    {
        glext_matrix_mode = mode;
        glMatrixMode(mode);
    }
}

void glPushMatrix_(void)
{
    if (glext_matrix_mode == GL_MODELVIEW)
    {
        if (glext_curr_depth_modelview < glext_max_depth_modelview)
        {
            glext_curr_depth_modelview++;
            glPushMatrix();
        }
    }

    if (glext_matrix_mode == GL_PROJECTION)
    {
        if (glext_curr_depth_projection < glext_max_depth_projection)
        {
            glext_curr_depth_projection++;
            glPushMatrix();
        }
    }
}

void glPopMatrix_(void)
{
    if (glext_matrix_mode == GL_MODELVIEW)
    {
        if (glext_curr_depth_modelview > 0)
        {
            glext_curr_depth_modelview--;
            glPopMatrix();
        }
    }

    if (glext_matrix_mode == GL_PROJECTION)
    {
        if (glext_curr_depth_projection > 0)
        {
            glext_curr_depth_projection--;
            glPopMatrix();
        }
    }
}

#endif

/*---------------------------------------------------------------------------*/
