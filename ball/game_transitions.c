/*
 * Copyright (C) 2026 Microsoft / Neverball authors / Ersohn Styne
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

#include "glext.h"
#include "vec3.h"
#include "geom.h"
#include "video.h"

#include "game_transitions.h"
#include "package_superwaifu.h"

/*---------------------------------------------------------------------------*/

#ifndef __EMSCRIPTEN__
struct game_transition {
    int state;

    int z_order;

    float transition_k;
    float transition_d;
};

static int game_transitions_state = 0;

static GLuint game_transitions_vbo;
static GLuint game_transitions_ebo;

static struct game_transition transitions;

static struct b_mtrl game_transition_base_mtrl = {
    { 1.0f, 1.0f, 1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f }, 0.0f, M_TRANSPARENT | M_CLAMP_S | M_CLAMP_T,
    "gui/transitions/transition_superwaifuball"
};

static int game_transition_mtrl = 0;
#endif

/*---------------------------------------------------------------------------*/

#define GAME_TRANSITION_VERTEX_SCALE 400.0f

int game_transitions_init(void)
{
#ifndef __EMSCRIPTEN__
    /*static const GLfloat verts[4][5] = {
        { -0.5f, -0.5f, 0.0f, 0.0f, 0.0f },
        { +0.5f, -0.5f, 0.0f, 1.0f, 0.0f },
        { -0.5f, +0.5f, 0.0f, 0.0f, 1.0f },
        { +0.5f, +0.5f, 0.0f, 1.0f, 1.0f },
    };*/

    static const GLfloat verts[4][5] = {
        { -GAME_TRANSITION_VERTEX_SCALE, -GAME_TRANSITION_VERTEX_SCALE, 0.0f, -GAME_TRANSITION_VERTEX_SCALE + 1, -GAME_TRANSITION_VERTEX_SCALE + 1 },
        { +GAME_TRANSITION_VERTEX_SCALE, -GAME_TRANSITION_VERTEX_SCALE, 0.0f, +GAME_TRANSITION_VERTEX_SCALE,     -GAME_TRANSITION_VERTEX_SCALE + 1 },
        { -GAME_TRANSITION_VERTEX_SCALE, +GAME_TRANSITION_VERTEX_SCALE, 0.0f, -GAME_TRANSITION_VERTEX_SCALE + 1, +GAME_TRANSITION_VERTEX_SCALE },
        { +GAME_TRANSITION_VERTEX_SCALE, +GAME_TRANSITION_VERTEX_SCALE, 0.0f, +GAME_TRANSITION_VERTEX_SCALE,     +GAME_TRANSITION_VERTEX_SCALE },
    };

    static const GLushort elems[4] = {
        0u, 1u, 2u, 3u
    };

    game_transition_mtrl = mtrl_cache(&game_transition_base_mtrl);

    glGenBuffers_(1,              &game_transitions_vbo);
    glBindBuffer_(GL_ARRAY_BUFFER, game_transitions_vbo);
    glBufferData_(GL_ARRAY_BUFFER, sizeof (verts), verts, GL_STATIC_DRAW);
    glBindBuffer_(GL_ARRAY_BUFFER, 0);

    glGenBuffers_(1,                      &game_transitions_ebo);
    glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER, game_transitions_ebo);
    glBufferData_(GL_ELEMENT_ARRAY_BUFFER, sizeof (elems), elems, GL_STATIC_DRAW);
    glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER, 0);

    game_transitions_state = (game_transitions_vbo != 0 && game_transitions_ebo != 0);

    if (!game_transitions_state) {
        if (game_transitions_vbo != 0) glDeleteBuffers_(1, &game_transitions_vbo);
        if (game_transitions_ebo != 0) glDeleteBuffers_(1, &game_transitions_ebo);
    }

    return game_transitions_state;
#else
    /* Not available in Emscripten! */

    return 0;
#endif
}

void game_transitions_quit(void)
{
#ifndef __EMSCRIPTEN__
    glDeleteBuffers_(1, &game_transitions_vbo);
    glDeleteBuffers_(1, &game_transitions_ebo);

    mtrl_free(game_transition_mtrl); game_transition_mtrl = 0;
#endif
}

int game_transitions_available(void)
{
#ifndef __EMSCRIPTEN__
    return fs_exists("gui/transitions/transition_superwaifuball.png") &&
           game_transitions_state && game_common_superwaifu_game_installed();
#else
    /* Not available in Emscripten! */

    return 0;
#endif
}

void game_transitions_step_fade(float dt)
{
#ifndef __EMSCRIPTEN__
    transitions.transition_k = CLAMP(0.0f,
                                     transitions.transition_k + transitions.transition_d * dt,
                                     1.0f);
#endif
}

void game_transitions_fade(float d)
{
#ifndef __EMSCRIPTEN__
    transitions.transition_d = d * 0.5f;
#endif
}

void game_transitions_fade_in(float d)
{
#ifndef __EMSCRIPTEN__
    transitions.transition_k = 1.0f;
    transitions.transition_d = d * 0.5f;
#endif
}

void game_transitions_draw(struct s_rend *rend)
{
#ifndef __EMSCRIPTEN__
    if (transitions.transition_k <= 0.0f) return;

    const float transition_icon_scale = video.device_w < video.device_h ?
                                        video.device_w * 0.75f :
                                        video.device_h * 0.75f;
    
    const float animated_scale = ((transitions.transition_k) < 0.5f ?
                                  21 + (20 * (-fsinf(transitions.transition_k * V_PI))) :
                                  fsinf(transitions.transition_k * V_PI));

    video_set_ortho();

    glBindBuffer_(GL_ARRAY_BUFFER,         game_transitions_vbo);
    glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER, game_transitions_ebo);

    glDisableClientState(GL_NORMAL_ARRAY);
    {
        glDisable(GL_DEPTH_TEST);

        glVertexPointer  (3, GL_FLOAT, sizeof (GLfloat) * 5, (GLvoid *) (                   0u));
        glTexCoordPointer(2, GL_FLOAT, sizeof (GLfloat) * 5, (GLvoid *) (sizeof (GLfloat) * 3u));

        r_apply_mtrl(rend, game_transition_mtrl);

        glPushMatrix();
        glTranslatef(video.device_w / 2, video.device_h / 2, 0.0f);
        glScalef(transition_icon_scale * MAX(animated_scale, 0.01f),
                 transition_icon_scale * MAX(animated_scale, 0.01f), 1.0f);
        glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, (GLvoid*) 0u);
        glPopMatrix();

        glEnable(GL_DEPTH_TEST);
    }
    glEnableClientState(GL_NORMAL_ARRAY);

    glBindBuffer_(GL_ARRAY_BUFFER,         0);
    glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER, 0);
#endif
}

int game_transitions_fadeout_finished(void)
{
#ifndef __EMSCRIPTEN__
    return transitions.transition_d > 0.0f &&
           transitions.transition_k >= 1.0f;
#else
    /* Not available in Emscripten! */

    return 0;
#endif
}

void game_transitions_set_zorder(int order)
{
#ifndef __EMSCRIPTEN__
    transitions.z_order = order;
#endif
}

int  game_transitions_get_zorder(void)
{
#ifndef __EMSCRIPTEN__
    return transitions.z_order;
#else
    /* Not available in Emscripten! */

    return 0;
#endif
}

void game_transitions_prep_scene(void)
{
#ifndef __EMSCRIPTEN__
    if (transitions.transition_k <= 0.0f) return;

    const int   center_x = ROUND(video.device_w / 2.0f);
    const float max_aspect_ratio = (21.0f / 9.0f);

    glPushScissor_(MAX(center_x - (video.device_h * max_aspect_ratio), 0), 0,
                   MIN((video.device_h * max_aspect_ratio), video.device_w), video.device_h);
#endif
}

void game_transitions_post_scene(void)
{
#ifndef __EMSCRIPTEN__
    if (transitions.transition_k <= 0.0f) return;

    glPopScissor_();
#endif
}

#undef GAME_TRANSITION_VERTEX_SCALE
