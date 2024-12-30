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

#ifndef GLEXT_H
#define GLEXT_H

/*---------------------------------------------------------------------------*/
/* Include the system OpenGL headers.                                        */

#ifndef __WII__
#ifdef _WIN32
#define NOMINMAX
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#ifndef _WIN32
#include <GL/glext.h>
#endif

/* Windows calling convention cruft. */

#ifndef APIENTRY
#define APIENTRY
#endif

#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif

/*---------------------------------------------------------------------------*/

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE                0x809D
#endif

#ifndef GL_TEXTURE0
#define GL_TEXTURE0                   0x84C0
#endif
#ifndef GL_TEXTURE1
#define GL_TEXTURE1                   0x84C1
#endif
#ifndef GL_TEXTURE2
#define GL_TEXTURE2                   0x84C2
#endif

#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER               0x8892
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER       0x8893
#endif

#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW                0x88E4
#endif
#ifndef GL_DYNAMIC_DRAW
#define GL_DYNAMIC_DRAW               0x88E8
#endif

#ifndef GL_POINT_SPRITE
#define GL_POINT_SPRITE               0x8861
#endif
#ifndef GL_COORD_REPLACE
#define GL_COORD_REPLACE              0x8862
#endif
#ifndef GL_POINT_SIZE_MIN
#define GL_POINT_SIZE_MIN             0x8126
#endif
#ifndef GL_POINT_SIZE_MAX
#define GL_POINT_SIZE_MAX             0x8127
#endif
#ifndef GL_POINT_DISTANCE_ATTENUATION
#define GL_POINT_DISTANCE_ATTENUATION 0x8129
#endif

#ifndef GL_SRC0_RGB
#define GL_SRC0_RGB                   0x8580
#endif
#ifndef GL_SRC1_RGB
#define GL_SRC1_RGB                   0x8581
#endif
#ifndef GL_SRC2_RGB
#define GL_SRC2_RGB                   0x8582
#endif
#ifndef GL_SRC0_ALPHA
#define GL_SRC0_ALPHA                 0x8588
#endif

#ifndef GL_DEPTH_STENCIL
#define GL_DEPTH_STENCIL              0x84F9
#endif
#ifndef GL_DEPTH24_STENCIL8
#define GL_DEPTH24_STENCIL8           0x88F0
#endif
#ifndef GL_UNSIGNED_INT_24_8
#define GL_UNSIGNED_INT_24_8          0x84FA
#endif
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER                0x8D40
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0          0x8CE0
#endif
#ifndef GL_DEPTH_ATTACHMENT
#define GL_DEPTH_ATTACHMENT           0x8D00
#endif
#ifndef GL_STENCIL_ATTACHMENT
#define GL_STENCIL_ATTACHMENT         0x8D20
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_COMPLETE       0x8CD5
#endif

#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER              0x8B31
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER            0x8B30
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS                0x8B82
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS             0x8B81
#endif
#ifndef GL_INFO_LOG_LENGTH
#define GL_INFO_LOG_LENGTH            0x8B84
#endif

#ifndef GL_CLAMP
#define GL_CLAMP                      0x2900
#endif

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE              0x812F
#endif

#ifndef GL_MAX_TEXTURE_UNITS
#define GL_MAX_TEXTURE_UNITS          0x84E2
#endif

#ifndef GL_COMBINE
#define GL_COMBINE                    0x8570
#endif

#ifndef GL_COMBINE_RGB
#define GL_COMBINE_RGB                0x8571
#endif

#ifndef GL_PREVIOUS
#define GL_PREVIOUS                   0x8578
#endif

#ifndef GL_OPERAND0_RGB
#define GL_OPERAND0_RGB               0x8590
#endif

#ifndef GL_OPERAND1_RGB
#define GL_OPERAND1_RGB               0x8591
#endif

#ifndef GL_OPERAND2_RGB
#define GL_OPERAND2_RGB               0x8592
#endif

#ifndef GL_COMBINE_ALPHA
#define GL_COMBINE_ALPHA              0x8572
#endif

#ifndef GL_OPERAND0_ALPHA
#define GL_OPERAND0_ALPHA             0x8598
#endif

#ifndef GL_INTERPOLATE
#define GL_INTERPOLATE                0x8575
#endif

#ifndef GL_PRIMARY_COLOR
#define GL_PRIMARY_COLOR              0x8577
#endif

#ifndef GL_LIGHT_MODEL_COLOR_CONTROL
#define GL_LIGHT_MODEL_COLOR_CONTROL  0x81F8
#endif

#ifndef GL_SEPARATE_SPECULAR_COLOR
#define GL_SEPARATE_SPECULAR_COLOR    0x81FA
#endif

#if _WIN32 || _WIN64
#ifndef GL_VIEWPORT_POSITION_W_SCALE_NV
#define GL_VIEWPORT_POSITION_W_SCALE_NV         0x937C
#endif

#ifndef GL_VIEWPORT_POSITION_W_SCALE_X_COEFF_NV
#define GL_VIEWPORT_POSITION_W_SCALE_X_COEFF_NV 0x937D
#endif

#ifndef GL_VIEWPORT_POSITION_W_SCALE_Y_COEFF_NV
#define GL_VIEWPORT_POSITION_W_SCALE_Y_COEFF_NV 0x937E
#endif

#ifndef GL_VERTEX_ARRAY_RANGE_NV
#define GL_VERTEX_ARRAY_RANGE_NV                0x851D
#endif

#ifndef GL_VERTEX_ARRAY_RANGE_LENGTH_NV
#define GL_VERTEX_ARRAY_RANGE_LENGTH_NV         0x851E
#endif

#ifndef GL_VERTEX_ARRAY_RANGE_VALID_NV
#define GL_VERTEX_ARRAY_RANGE_VALID_NV          0x851F
#endif

#ifndef GL_MAX_VERTEX_ARRAY_RANGE_ELEMENT_NV
#define GL_MAX_VERTEX_ARRAY_RANGE_ELEMENT_NV    0x8520
#endif

#ifndef GL_VERTEX_ARRAY_RANGE_POINTER_NV
#define GL_VERTEX_ARRAY_RANGE_POINTER_NV        0x8521
#endif

#ifndef GL_TERMINATE_SEQUENCE_COMMAND_NV
#define GL_TERMINATE_SEQUENCE_COMMAND_NV        0x0000
#endif

#ifndef GL_NOP_COMMAND_NV
#define GL_NOP_COMMAND_NV                       0x0001
#endif

#ifndef GL_DRAW_ELEMENTS_COMMAND_NV
#define GL_DRAW_ELEMENTS_COMMAND_NV             0x0002
#endif

#ifndef GL_DRAW_ARRAYS_COMMAND_NV
#define GL_DRAW_ARRAYS_COMMAND_NV               0x0003
#endif

#ifndef GL_DRAW_ELEMENTS_STRIP_COMMAND_NV
#define GL_DRAW_ELEMENTS_STRIP_COMMAND_NV       0x0004
#endif

#ifndef GL_DRAW_ARRAYS_STRIP_COMMAND_NV
#define GL_DRAW_ARRAYS_STRIP_COMMAND_NV         0x0005
#endif

#ifndef GL_DRAW_ELEMENTS_INSTANCED_COMMAND_NV
#define GL_DRAW_ELEMENTS_INSTANCED_COMMAND_NV   0x0006
#endif

#ifndef GL_DRAW_ARRAYS_INSTANCED_COMMAND_NV
#define GL_DRAW_ARRAYS_INSTANCED_COMMAND_NV     0x0007
#endif

#ifndef GL_ELEMENT_ADDRESS_COMMAND_NV
#define GL_ELEMENT_ADDRESS_COMMAND_NV           0x0008
#endif

#ifndef GL_ATTRIBUTE_ADDRESS_COMMAND_NV
#define GL_ATTRIBUTE_ADDRESS_COMMAND_NV         0x0009
#endif

#ifndef GL_UNIFORM_ADDRESS_COMMAND_NV
#define GL_UNIFORM_ADDRESS_COMMAND_NV           0x000A
#endif

#ifndef GL_BLEND_COLOR_COMMAND_NV
#define GL_BLEND_COLOR_COMMAND_NV               0x000B
#endif

#ifndef GL_STENCIL_REF_COMMAND_NV
#define GL_STENCIL_REF_COMMAND_NV               0x000C
#endif

#ifndef GL_LINE_WIDTH_COMMAND_NV
#define GL_LINE_WIDTH_COMMAND_NV                0x000D
#endif

#ifndef GL_POLYGON_OFFSET_COMMAND_NV
#define GL_POLYGON_OFFSET_COMMAND_NV            0x000E
#endif

#ifndef GL_ALPHA_REF_COMMAND_NV
#define GL_ALPHA_REF_COMMAND_NV                 0x000F
#endif

#ifndef GL_VIEWPORT_COMMAND_NV
#define GL_VIEWPORT_COMMAND_NV                  0x0010
#endif

#ifndef GL_SCISSOR_COMMAND_NV
#define GL_SCISSOR_COMMAND_NV                   0x0011
#endif

#ifndef GL_FRONT_FACE_COMMAND_NV
#define GL_FRONT_FACE_COMMAND_NV                0x0012
#endif

#endif
#else
#include "wiigl.h"
#endif

/*---------------------------------------------------------------------------*/

#define glext_check(string) glext_check_ext(string)

int glext_check_renderer(const char *renderer);
int glext_check_ext(const char *);
int glext_init(void);

/*---------------------------------------------------------------------------*/

/* Exercise extreme paranoia in defining a cross-platform OpenGL interface.  */
/* If we're compiling against OpenGL ES then we must assume native linkage   */
/* of the extensions we use. Otherwise, GetProc them regardless of whether   */
/* they need it or not.                                                      */

#if ENABLE_OPENGL_ES || \
    defined(__EMSCRIPTEN__)

#define ENABLE_OPENGLES 1

#define glClientActiveTexture_ glClientActiveTexture
#define glActiveTexture_       glActiveTexture
#define glGenBuffers_          glGenBuffers
#define glBindBuffer_          glBindBuffer
#define glBufferData_          glBufferData
#define glBufferSubData_       glBufferSubData
#define glDeleteBuffers_       glDeleteBuffers
#define glIsBuffer_            glIsBuffer
#define glPointParameterfv_    glPointParameterfv
#define glPointParameterf_     glPointParameterf

#ifdef __EMSCRIPTEN__
#define glOrtho_               glOrtho
#else
#define glOrtho_               glOrthof
#endif

#define glStringMarker_(s) ((void) (s))

#elif !defined(__WII__) /* No native linkage?  Define the extension API. */

#define glOrtho_               glOrtho

/*---------------------------------------------------------------------------*/
/* ARB_multitexture                                                          */

typedef void (APIENTRYP PFNGLACTIVETEXTURE_PROC)       (GLenum);
typedef void (APIENTRYP PFNGLCLIENTACTIVETEXTURE_PROC) (GLenum);

extern PFNGLCLIENTACTIVETEXTURE_PROC glClientActiveTexture_;
extern PFNGLACTIVETEXTURE_PROC       glActiveTexture_;

/*---------------------------------------------------------------------------*/
/* ARB_vertex_buffer_object                                                  */

typedef void      (APIENTRYP PFNGLGENBUFFERS_PROC)    (GLsizei, GLuint *);
typedef void      (APIENTRYP PFNGLBINDBUFFER_PROC)    (GLenum, GLuint);
typedef void      (APIENTRYP PFNGLBUFFERDATA_PROC)    (GLenum, long, const GLvoid *, GLenum);
typedef void      (APIENTRYP PFNGLBUFFERSUBDATA_PROC) (GLenum, long, long, const GLvoid *);
typedef void      (APIENTRYP PFNGLDELETEBUFFERS_PROC) (GLsizei, const GLuint *);
typedef GLboolean (APIENTRYP PFNGLISBUFFER_PROC)      (GLuint);

extern PFNGLGENBUFFERS_PROC    glGenBuffers_;
extern PFNGLBINDBUFFER_PROC    glBindBuffer_;
extern PFNGLBUFFERDATA_PROC    glBufferData_;
extern PFNGLBUFFERSUBDATA_PROC glBufferSubData_;
extern PFNGLDELETEBUFFERS_PROC glDeleteBuffers_;
extern PFNGLISBUFFER_PROC      glIsBuffer_;

/*---------------------------------------------------------------------------*/
/* ARB_point_parameters                                                      */

typedef void (APIENTRYP PFNGLPOINTPARAMETERFV_PROC) (GLenum, const GLfloat *);
typedef void (APIENTRYP PFNGLPOINTPARAMETERF_PROC)  (GLenum, const GLfloat);

extern PFNGLPOINTPARAMETERFV_PROC glPointParameterfv_;
extern PFNGLPOINTPARAMETERF_PROC  glPointParameterf_;

/*---------------------------------------------------------------------------*/
/* OpenGL Shading Language                                                   */

typedef void   (APIENTRYP PFNGLGETSHADERIV_PROC)        (GLuint, GLenum, GLint *);
typedef void   (APIENTRYP PFNGLGETSHADERINFOLOG_PROC)   (GLuint, GLsizei, GLsizei *, char *);
typedef void   (APIENTRYP PFNGLGETPROGRAMIV_PROC)       (GLuint, GLenum, GLint *);
typedef void   (APIENTRYP PFNGLGETPROGRAMINFOLOG_PROC)  (GLuint, GLsizei, GLsizei *, char *);
typedef GLuint (APIENTRYP PFNGLCREATESHADER_PROC)       (GLenum);
typedef GLuint (APIENTRYP PFNGLCREATEPROGRAM_PROC)      (void);
typedef void   (APIENTRYP PFNGLSHADERSOURCE_PROC)       (GLuint, GLsizei, const char * const *, const GLint *);
typedef void   (APIENTRYP PFNGLCOMPILESHADER_PROC)      (GLuint);
typedef void   (APIENTRYP PFNGLDELETESHADER_PROC)       (GLuint);
typedef void   (APIENTRYP PFNGLDELETEPROGRAM_PROC)      (GLuint);
typedef void   (APIENTRYP PFNGLATTACHSHADER_PROC)       (GLuint, GLuint);
typedef void   (APIENTRYP PFNGLLINKPROGRAM_PROC)        (GLuint);
typedef void   (APIENTRYP PFNGLUSEPROGRAM_PROC)         (GLuint);
typedef GLint  (APIENTRYP PFNGLGETUNIFORMLOCATION_PROC) (GLuint, const char *);
typedef void   (APIENTRYP PFNGLUNIFORM1F_PROC)          (GLint, GLfloat);
typedef void   (APIENTRYP PFNGLUNIFORM2F_PROC)          (GLint, GLfloat, GLfloat);
typedef void   (APIENTRYP PFNGLUNIFORM3F_PROC)          (GLint, GLfloat, GLfloat, GLfloat);
typedef void   (APIENTRYP PFNGLUNIFORM4F_PROC)          (GLint, GLfloat, GLfloat, GLfloat, GLfloat);

extern PFNGLGETSHADERIV_PROC         glGetShaderiv_;
extern PFNGLGETSHADERINFOLOG_PROC    glGetShaderInfoLog_;
extern PFNGLGETPROGRAMIV_PROC        glGetProgramiv_;
extern PFNGLGETPROGRAMINFOLOG_PROC   glGetProgramInfoLog_;
extern PFNGLCREATESHADER_PROC        glCreateShader_;
extern PFNGLCREATEPROGRAM_PROC       glCreateProgram_;
extern PFNGLSHADERSOURCE_PROC        glShaderSource_;
extern PFNGLCOMPILESHADER_PROC       glCompileShader_;
extern PFNGLDELETESHADER_PROC        glDeleteShader_;
extern PFNGLDELETEPROGRAM_PROC       glDeleteProgram_;
extern PFNGLATTACHSHADER_PROC        glAttachShader_;
extern PFNGLLINKPROGRAM_PROC         glLinkProgram_;
extern PFNGLUSEPROGRAM_PROC          glUseProgram_;
extern PFNGLGETUNIFORMLOCATION_PROC  glGetUniformLocation_;
extern PFNGLUNIFORM1F_PROC           glUniform1f_;
extern PFNGLUNIFORM2F_PROC           glUniform2f_;
extern PFNGLUNIFORM3F_PROC           glUniform3f_;
extern PFNGLUNIFORM4F_PROC           glUniform4f_;

/*---------------------------------------------------------------------------*/
/* ARB_framebuffer_object                                                    */

typedef void   (APIENTRYP PFNGLBINDFRAMEBUFFER_PROC)        (GLenum, GLuint);
typedef void   (APIENTRYP PFNGLDELETEFRAMEBUFFERS_PROC)     (GLsizei, const GLuint *);
typedef void   (APIENTRYP PFNGLGENFRAMEBUFFERS_PROC)        (GLsizei, GLuint *);
typedef void   (APIENTRYP PFNGLFRAMEBUFFERTEXTURE2D_PROC)   (GLenum, GLenum, GLenum, GLuint, GLint);
typedef GLenum (APIENTRYP PFNGLCHECKFRAMEBUFFERSTATUS_PROC) (GLenum);

extern PFNGLBINDFRAMEBUFFER_PROC        glBindFramebuffer_;
extern PFNGLDELETEFRAMEBUFFERS_PROC     glDeleteFramebuffers_;
extern PFNGLGENFRAMEBUFFERS_PROC        glGenFramebuffers_;
extern PFNGLFRAMEBUFFERTEXTURE2D_PROC   glFramebufferTexture2D_;
extern PFNGLCHECKFRAMEBUFFERSTATUS_PROC glCheckFramebufferStatus_;

/*---------------------------------------------------------------------------*/
/* GREMEDY_string_marker                                                     */

typedef void (APIENTRYP PFNGLSTRINGMARKERGREMEDY_PROC) (GLsizei, const void *);

extern PFNGLSTRINGMARKERGREMEDY_PROC glStringMarkerGREMEDY_;

#define glStringMarker_(s)      \
    if (glStringMarkerGREMEDY_) \
        glStringMarkerGREMEDY_(0, (s))

#if ENABLE_GL_NV
/*---------------------------------------------------------------------------*/
/* GL_NV_clip_space_w_scaling                                                */

typedef void (APIENTRYP PFNGLVIEWPORTPOSITIONWSCALENV_PROC) (GLuint id, GLfloat x, GLfloat y);

extern PFNGLVIEWPORTPOSITIONWSCALENV_PROC glViewportPositionWScaleNV_;

/*---------------------------------------------------------------------------*/
/* GL_NV_occlusion_query                                                     */

typedef void      (APIENTRYP PFNGLGENOCCLUSIONQUERIESNV_PROC)    (GLsizei n, GLuint *ids);
typedef void      (APIENTRYP PFNGLDELETEOCCLUSIONQUERIESNV_PROC) (GLsizei n, const GLuint *ids);
typedef GLboolean (APIENTRYP PFNGLISOCCLUSIONQUERYNV_PROC)       (GLuint id);
typedef void      (APIENTRYP PFNGLBEGINOCCLUSIONQUERYNV_PROC)    (GLuint id);
typedef void      (APIENTRYP PFNGLENDOCCLUSIONQUERYNV_PROC)      (void);
typedef void      (APIENTRYP PFNGLGETOCCLUSIONQUERYIVNV_PROC)    (GLuint id, GLenum pname, GLint *params);
typedef void      (APIENTRYP PFNGLGETOCCLUSIONQUERYUIVNV_PROC)   (GLuint id, GLenum pname, GLuint *params);

extern PFNGLGENOCCLUSIONQUERIESNV_PROC    glGenOcclusionQueriesNV_;
extern PFNGLDELETEOCCLUSIONQUERIESNV_PROC glDeleteOcclusionQueriesNV_;
extern PFNGLISOCCLUSIONQUERYNV_PROC       glIsOcclusionQueryNV_;
extern PFNGLBEGINOCCLUSIONQUERYNV_PROC    glBeginOcclusionQueryNV_;
extern PFNGLENDOCCLUSIONQUERYNV_PROC      glEndOcclusionQueryNV_;
extern PFNGLGETOCCLUSIONQUERYIVNV_PROC    glGetOcclusionQueryivNV_;
extern PFNGLGETOCCLUSIONQUERYUIVNV_PROC   glGetOcclusionQueryuivNV_;

/*---------------------------------------------------------------------------*/
/* GL_NV_vertex_array_range                                                  */

typedef void (APIENTRYP PFNGLFLUSHVERTEXARRAYRANGENV_PROC) (void);
typedef void (APIENTRYP PFNGLVERTEXARRAYRANGENV_PROC)      (GLsizei length, const void *pointer);

extern PFNGLFLUSHVERTEXARRAYRANGENV_PROC glFlushVertexArrayRangeNV_;
extern PFNGLVERTEXARRAYRANGENV_PROC      glVertexArrayRangeNV_;

/*---------------------------------------------------------------------------*/
/* GL_NV_command_list                                                        */

typedef void      (APIENTRYP PFNGLCREATESTATESNV_PROC)                 (GLsizei n, GLuint *states);
typedef void      (APIENTRYP PFNGLDELETESTATESNV_PROC)                 (GLsizei n, const GLuint *states);
typedef GLboolean (APIENTRYP PFNGLISSTATENV_PROC)                      (GLuint state);
typedef void      (APIENTRYP PFNGLSTATECAPTURENV_PROC)                 (GLuint state, GLenum mode);
typedef GLuint    (APIENTRYP PFNGLGETCOMMANDHEADERNV_PROC)             (GLenum tokenID, GLuint size);
typedef GLushort  (APIENTRYP PFNGLGETSTAGEINDEXNV_PROC)                (GLenum shadertype);
typedef void      (APIENTRYP PFNGLDRAWCOMMANDSNV_PROC)                 (GLenum primitiveMode, GLuint buffer, const GLintptr *indirects, const GLsizei *sizes, GLuint count);
typedef void      (APIENTRYP PFNGLDRAWCOMMANDSADDRESSNV_PROC)          (GLenum primitiveMode, const GLuint64 *indirects, const GLsizei *sizes, GLuint count);
typedef void      (APIENTRYP PFNGLDRAWCOMMANDSSTATESNV_PROC)           (GLuint buffer, const GLintptr *indirects, const GLsizei *sizes, const GLuint *states, const GLuint *fbos, GLuint count);
typedef void      (APIENTRYP PFNGLDRAWCOMMANDSSTATESADDRESSNV_PROC)    (const GLuint64* indirects, const GLsizei *sizes, const GLuint *states, const GLuint *fbos, GLuint count);
typedef void      (APIENTRYP PFNGLCREATECOMMANDLISTSNV_PROC)           (GLsizei n, GLuint *lists);
typedef void      (APIENTRYP PFNGLDELETECOMMANDLISTSNV_PROC)           (GLsizei n, const GLuint *lists);
typedef GLboolean (APIENTRYP PFNGLISCOMMANDLISTNV_PROC)                (GLuint list);
typedef void      (APIENTRYP PFNGLLISTDRAWCOMMANDSSTATESCLIENTNV_PROC) (GLuint list, GLuint segment, const void **indirects, const GLsizei *sizes, const GLuint *states, const GLuint *fbos, GLuint count);
typedef void      (APIENTRYP PFNGLCOMMANDLISTSEGMENTSNV_PROC)          (GLuint list, GLuint segments);
typedef void      (APIENTRYP PFNGLCOMPILECOMMANDLISTNV_PROC)           (GLuint list);
typedef void      (APIENTRYP PFNGLCALLCOMMANDLISTNV_PROC)              (GLuint list);

extern PFNGLCREATESTATESNV_PROC                 glCreateStatesNV_;
extern PFNGLDELETESTATESNV_PROC                 glDeleteStatesNV_;
extern PFNGLISSTATENV_PROC                      glIsStateNV_;
extern PFNGLSTATECAPTURENV_PROC                 glStateCaptureNV_;
extern PFNGLGETCOMMANDHEADERNV_PROC             glGetCommandHeaderNV_;
extern PFNGLGETSTAGEINDEXNV_PROC                glGetStageIndexNV_;
extern PFNGLDRAWCOMMANDSNV_PROC                 glDrawCommandsNV_;
extern PFNGLDRAWCOMMANDSADDRESSNV_PROC          glDrawCommandsAddressNV_;
extern PFNGLDRAWCOMMANDSSTATESNV_PROC           glDrawCommandsStatesNV_;
extern PFNGLDRAWCOMMANDSSTATESADDRESSNV_PROC    glDrawCommandsStatesAddressNV_;
extern PFNGLCREATECOMMANDLISTSNV_PROC           glCreateCommandListsNV_;
extern PFNGLDELETECOMMANDLISTSNV_PROC           glDeleteCommandListsNV_;
extern PFNGLISCOMMANDLISTNV_PROC                glIsCommandListNV_;
extern PFNGLLISTDRAWCOMMANDSSTATESCLIENTNV_PROC glListDrawCommandsStatesClientNV_;
extern PFNGLCOMMANDLISTSEGMENTSNV_PROC          glCommandListSegmentsNV_;
extern PFNGLCOMPILECOMMANDLISTNV_PROC           glCompileCommandListNV_;
extern PFNGLCALLCOMMANDLISTNV_PROC              glCallCommandListNV_;

#endif

/*---------------------------------------------------------------------------*/
#endif /* ENABLE_OPENGLES || defined(__EMSCRIPTEN__) */

void glClipPlane4f_(GLenum, GLfloat, GLfloat, GLfloat, GLfloat);
void glBindTexture_(GLenum target, GLuint texture);

void glToggleWireframe_(void);
void glSetWireframe_   (int);

/*---------------------------------------------------------------------------*/

struct gl_info
{
    int max_texture_units;
    int max_texture_size;

    unsigned int texture_filter_anisotropic : 1;
    unsigned int shader_objects             : 1;
    unsigned int framebuffer_object         : 1;
    unsigned int string_marker              : 1;

    unsigned int wireframe:1;
};

extern struct gl_info gli;

/*---------------------------------------------------------------------------*/
#endif
