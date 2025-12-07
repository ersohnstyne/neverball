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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

#if !_MSC_VER
#include <unistd.h>
#endif

 /*
 * HACK: Used with console version
 */
#include "console_control_gui.h"

#include "dbg_config.h"

#include "config.h"
#include "video.h"
#include "glext.h"
#include "image.h"
#include "vec3.h"
#include "gui.h"
#include "common.h"
#include "font.h"
#include "theme.h"
#include "log.h"
#include "lang.h"
#include "ease.h"
#include "transition.h"

#include "fs.h"

#include "audio.h"

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

#if _DEBUG && _MSC_VER
#ifndef _CRTDBG_MAP_ALLOC
#pragma message(__FILE__": Missing _CRT_MAP_ALLOC, recreate: _CRTDBG_MAP_ALLOC + crtdbg.h")
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#endif

#if ENABLE_MOTIONBLUR!=0
#define GUI_COLOR4UB \
        gui_wht[0], gui_wht[1], gui_wht[2], ROUND((gui_wht[3] * (widget[id].alpha_slide * widget[id].alpha)) * video_motionblur_alpha_get())
#else
#define GUI_COLOR4UB \
        gui_wht[0], gui_wht[1], gui_wht[2], ROUND((gui_wht[3] * (widget[id].alpha_slide * widget[id].alpha)))
#endif

/*---------------------------------------------------------------------------*/

/* Very pure colors for the GUI. I was watching BANZAI when I designed this. */

const GLubyte gui_wht[4] = { 0xFF, 0xFF, 0xFF, 0xFF };  /* White    */
const GLubyte gui_yel[4] = { 0xFF, 0xFF, 0x00, 0xFF };  /* Yellow   */
const GLubyte gui_cya[4] = { 0x00, 0xFF, 0xFF, 0xFF };  /* Cyan     */
const GLubyte gui_twi[4] = { 0x80, 0x00, 0xFF, 0xFF };  /* Twilight */
const GLubyte gui_vio[4] = { 0xFF, 0x00, 0xFF, 0xFF };  /* Violet   */
const GLubyte gui_pnk[4] = { 0xFF, 0x55, 0xFF, 0xFF };  /* Pink     */
const GLubyte gui_red[4] = { 0xFF, 0x00, 0x00, 0xFF };  /* Red      */
const GLubyte gui_grn[4] = { 0x00, 0xFF, 0x00, 0xFF };  /* Green    */
const GLubyte gui_blu[4] = { 0x00, 0x00, 0xFF, 0xFF };  /* Blue     */
const GLubyte gui_brn[4] = { 0xCB, 0x4A, 0x00, 0xFF };  /* Brown    */
const GLubyte gui_blk[4] = { 0x00, 0x00, 0x00, 0xFF };  /* Black    */
const GLubyte gui_gry[4] = { 0x55, 0x55, 0x55, 0xFF };  /* Gray     */
const GLubyte gui_shd[4] = { 0x00, 0x00, 0x00, 0x80 };  /* Shadow   */

const GLubyte gui_wht2[4] = { 0xFF, 0xFF, 0xFF, 0x60 }; /* Transparent white */

/*---------------------------------------------------------------------------*/

#define WIDGET_MAX 512 /* Twice as high (previous is 256) */

#define GUI_FREE   0
#define GUI_HARRAY 1
#define GUI_VARRAY 2
#define GUI_HSTACK 3
#define GUI_VSTACK 4
#define GUI_FILLER 5
#define GUI_IMAGE  6
#define GUI_LABEL  7
#define GUI_COUNT  8
#define GUI_CLOCK  9
#define GUI_SPACE  10
#define GUI_BUTTON 11
#define GUI_ROOT   12

#define GUI_STATE  1
#define GUI_FILL   2
#define GUI_HILITE 4
#define GUI_RECT   8
#define GUI_LAYOUT 16
#define GUI_CLIP   32
#define GUI_OFFSET 64

/* Default: 8 lines */
#define GUI_LINES 16

/*---------------------------------------------------------------------------*/

struct widget
{
    int     type;
    int     flags;
    int     token;
    int     value;
    int     font;
    int     size;
    int     rect;

    char   *text;

    int     init_value;
    char   *init_text;

    int     layout_xd;
    int     layout_yd;

    const GLubyte *color0;
    const GLubyte *color1;

    int     x, y;
    int     w, h;
    int     car;
    int     cdr;

    GLuint  image;
    GLfloat pulse_scale;

    int     text_w;
    int     text_h;

    enum trunc trunc;

    float   alpha;
    float   alpha_slide;
    int     animation_direction;

    float offset_init_x;
    float offset_init_y;

    float offset_x;
    float offset_y;

    int   slide_flags;
    float slide_delay;
    float slide_dur;
    float slide_time;

    unsigned int hidden:1;
};

/*---------------------------------------------------------------------------*/

/* GUI widget state */

static struct widget widget[WIDGET_MAX];
static int           active;
static int           hovered;
static int           clicked;
static int           padding;
static int           borders[4];

/* Digit widgets for the HUD. */

static int digit_id[FONT_SIZE_MAX][11];

/* Cursor image. */

static int cursor_id = 0;
static int cursor_st = 0;

/* GUI theme. */

static struct theme curr_theme;

#define FUNC_VOID_CHECK_LIMITS(_id)                           \
    do if (_id < 0 || _id > WIDGET_MAX) {                     \
        log_errorf("Widget index out of bounds!: %d\n", _id); \
        return;                                               \
    } while (0)

#define FUNC_INT_CHECK_LIMITS(_id)                            \
    do if (_id < 0 || _id > WIDGET_MAX) {                     \
        log_errorf("Widget index out of bounds!: %d\n", _id); \
        return _id;                                           \
    } while (0)

/*---------------------------------------------------------------------------*/

static int gui_hot(int id)
{
    return (widget[id].flags & GUI_STATE) && !widget[id].hidden;
}

/*---------------------------------------------------------------------------*/

/* Vertex buffer definitions for widget rendering. */

/* Vertex count */

#define RECT_VERT  16
#define TEXT_VERT  8
#define IMAGE_VERT 4

#define WIDGET_VERT (RECT_VERT + MAX(TEXT_VERT, IMAGE_VERT))

/* Element count */

#define RECT_ELEM 28

#define WIDGET_ELEM (RECT_ELEM)

struct vert
{
    GLubyte c[4];
    GLfloat u[2];
    GLshort p[2];
};

static struct vert vert_buf[WIDGET_MAX * WIDGET_VERT];
static GLuint      vert_vbo = 0;
static GLuint      vert_ebo = 0;

/*---------------------------------------------------------------------------*/

static void set_vert(struct vert *v, int x, int y,
                     GLfloat s, GLfloat t, const GLubyte *c)
{
    v->c[0] = c[0];
    v->c[1] = c[1];
    v->c[2] = c[2];
    v->c[3] = c[3];
    v->u[0] = s;
    v->u[1] = t;
    v->p[0] = x;
    v->p[1] = y;
}

/*---------------------------------------------------------------------------*/

static void draw_enable(GLboolean c, GLboolean u, GLboolean p)
{
    glBindBuffer_(GL_ARRAY_BUFFER,         vert_vbo);
    glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER, vert_ebo);

    if (c)
    {
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer   (4, GL_UNSIGNED_BYTE, sizeof (struct vert),
                                  (GLvoid *) offsetof (struct vert, c));
    }
    if (u)
    {
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT,         sizeof (struct vert),
                                  (GLvoid *) offsetof (struct vert, u));
    }
    if (p)
    {
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer  (2, GL_SHORT,         sizeof (struct vert),
                                  (GLvoid *) offsetof (struct vert, p));
    }
}

static void draw_rect(int id)
{
    glDrawElements(GL_TRIANGLE_STRIP, RECT_ELEM, GL_UNSIGNED_SHORT,
                   (const GLvoid *) (id * WIDGET_ELEM * sizeof (GLushort)));
}

static void draw_text(int id)
{
    glDrawArrays(GL_TRIANGLE_STRIP, id * WIDGET_VERT + RECT_VERT, TEXT_VERT);
}

static void draw_image(int id)
{
    glDrawArrays(GL_TRIANGLE_STRIP, id * WIDGET_VERT + RECT_VERT, IMAGE_VERT);
}

static void draw_disable(void)
{
    glBindBuffer_(GL_ARRAY_BUFFER,         0);
    glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER, 0);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
}

/*---------------------------------------------------------------------------*/

/*
 * Generate vertices for a 3x3 rectangle. Vertices are arranged
 * top-to-bottom and left-to-right, triangle strips are arranged
 * left-to-right and top-to-bottom (one strip per row). Degenerate
 * triangles (two extra indices per stitch) are inserted for a
 * continous strip.
 */

static const GLushort rect_elem_base[RECT_ELEM] = {
    0, 1, 4, 5, 8,  9,  12, 13, 13, 1, /* Top    */
    1, 2, 5, 6, 9,  10, 13, 14, 14, 2, /* Middle */
    2, 3, 6, 7, 10, 11, 14, 15         /* Bottom */
};

static void gui_geom_rect(int id, int x, int y, int w, int h, int f)
{
    GLushort rect_elem[RECT_ELEM];

    struct vert *v = vert_buf + id * WIDGET_VERT;
    struct vert *p = v;

    int X[4];
    int Y[4];

    int i, j;

    /* Generate vertex and element data for the widget's rectangle. */

    X[0] = x;
    X[1] = x +     ((f & GUI_W) ? borders[0] : 0);
    X[2] = x + w - ((f & GUI_E) ? borders[1] : 0);
    X[3] = x + w;

    Y[0] = y + h;
    Y[1] = y + h - ((f & GUI_N) ? borders[2] : 0);
    Y[2] = y +     ((f & GUI_S) ? borders[3] : 0);
    Y[3] = y;

    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            set_vert(p++, X[i], Y[j], curr_theme.s[i], curr_theme.t[j], gui_wht);

    for (i = 0; i < RECT_ELEM; i++)
        rect_elem[i] = id * WIDGET_VERT + rect_elem_base[i];

    /* Copy this off to the VBOs. */

    glBindBuffer_   (GL_ARRAY_BUFFER, vert_vbo);
    glBufferSubData_(GL_ARRAY_BUFFER,
                     id * WIDGET_VERT * sizeof (struct vert),
                            RECT_VERT * sizeof (struct vert), v);
    glBindBuffer_   (GL_ARRAY_BUFFER, 0);

    glBindBuffer_   (GL_ELEMENT_ARRAY_BUFFER, vert_ebo);
    glBufferSubData_(GL_ELEMENT_ARRAY_BUFFER,
                     id * WIDGET_ELEM * sizeof (GLushort),
                            RECT_ELEM * sizeof (GLushort), rect_elem);
    glBindBuffer_   (GL_ELEMENT_ARRAY_BUFFER, 0);
}

static void gui_geom_text(int id, int x, int y, int w, int h,
                          const GLubyte *c0, const GLubyte *c1)
{
    struct vert *v = vert_buf + id * WIDGET_VERT + RECT_VERT;

    /* Assume the applied texture size is rect size rounded to power-of-two. */

    int W;
    int H;

    image_size(&W, &H, w, h);

    if (w > 0 && h > 0 && W > 0 && H > 0)
    {
        const int d = h / 16;  /* Shadow offset */

        const int ww = ((W - w) % 2) ? w + 1 : w;
        const int hh = ((H - h) % 2) ? h + 1 : h;

        const GLfloat s0 = 0.5f * (W - ww) / W;
        const GLfloat t0 = 0.5f * (H - hh) / H;
        const GLfloat s1 = 1.0f - s0;
        const GLfloat t1 = 1.0f - t0;

        GLubyte color[4];

        color[0] = gui_shd[0];
        color[1] = gui_shd[1];
        color[2] = gui_shd[2];
        color[3] = c0[3] < 0xFF ? (GLubyte)(c0[3] * 0.5f) : gui_shd[3];

        /* Generate vertex data for the colored text and its shadow. */

        set_vert(v + 0, x      + d, y + hh - d, s0, t0, color);
        set_vert(v + 1, x      + d, y      - d, s0, t1, color);
        set_vert(v + 2, x + ww + d, y + hh - d, s1, t0, color);
        set_vert(v + 3, x + ww + d, y      - d, s1, t1, color);

        set_vert(v + 4, x,          y + hh,     s0, t0, c1);
        set_vert(v + 5, x,          y,          s0, t1, c0);
        set_vert(v + 6, x + ww,     y + hh,     s1, t0, c1);
        set_vert(v + 7, x + ww,     y,          s1, t1, c0);

    }
    else memset(v, 0, TEXT_VERT * sizeof (struct vert));

    /* Copy this off to the VBO. */

    glBindBuffer_   (GL_ARRAY_BUFFER, vert_vbo);
    glBufferSubData_(GL_ARRAY_BUFFER,
                     (id * WIDGET_VERT + RECT_VERT) * sizeof (struct vert),
                                         TEXT_VERT  * sizeof (struct vert), v);
    glBindBuffer_   (GL_ARRAY_BUFFER, 0);
}

static void gui_geom_image(int id, int x, int y, int w, int h, int f)
{
    struct vert *v = vert_buf + id * WIDGET_VERT + RECT_VERT;

    int X[2];
    int Y[2];

    /* Trace inner vertices of the background rectangle. */

    X[0] = x +     ((f & GUI_W) ? borders[0] : 0);
    X[1] = x + w - ((f & GUI_E) ? borders[1] : 0);

    Y[0] = y + h - ((f & GUI_N) ? borders[2] : 0);
    Y[1] = y +     ((f & GUI_S) ? borders[3] : 0);

    set_vert(v + 0, X[0], Y[0], 0.0f, 1.0f, gui_wht);
    set_vert(v + 1, X[0], Y[1], 0.0f, 0.0f, gui_wht);
    set_vert(v + 2, X[1], Y[0], 1.0f, 1.0f, gui_wht);
    set_vert(v + 3, X[1], Y[1], 1.0f, 0.0f, gui_wht);

    /* Copy this off to the VBO. */

    glBindBuffer_   (GL_ARRAY_BUFFER, vert_vbo);
    glBufferSubData_(GL_ARRAY_BUFFER,
                     (id * WIDGET_VERT + RECT_VERT) * sizeof (struct vert),
                                         IMAGE_VERT * sizeof (struct vert), v);
    glBindBuffer_   (GL_ARRAY_BUFFER, 0);
}

static void gui_geom_widget(int id, int flags)
{
    int jd;

    int w = widget[id].w;
    int h = widget[id].h;

    int W = widget[id].text_w;
    int H = widget[id].text_h;
    int R = widget[id].rect;

    const GLubyte *c0 = widget[id].color0;
    const GLubyte *c1 = widget[id].color1;

    if ((widget[id].flags & GUI_RECT) && !(flags & GUI_RECT))
    {
        gui_geom_rect(id, -w / 2, -h / 2, w, h, R);
        flags |= GUI_RECT;
    }

    switch (widget[id].type)
    {
        case GUI_FILLER:
        case GUI_SPACE:
            break;

        case GUI_HARRAY:
        case GUI_VARRAY:
        case GUI_HSTACK:
        case GUI_VSTACK:
            for (jd = widget[id].car; jd; jd = widget[jd].cdr)
                gui_geom_widget(jd, flags);

            break;

        case GUI_IMAGE:
            gui_geom_image(id, -w / 2, -h / 2, w, h, R);
            break;

        case GUI_BUTTON:
        case GUI_LABEL:
            /* Handled by gui_render_text(). */
            break;

        default:
            gui_geom_text(id, -W / 2, -H / 2, W, H, c0, c1);
            break;
    }
}

/*---------------------------------------------------------------------------*/

#define FONT_MAX 4

static struct font fonts[FONT_MAX];
static int         fontc;

static const int font_sizes_scale[FONT_SIZE_MAX] = {
    52, /* GUI_TNY */
    44, /* GUI_XS  */
    26, /* GUI_SML */
    20, /* GUI_TCH */
    13, /* GUI_MED */
    7,  /* GUI_LRG */
};

static int font_sizes[FONT_SIZE_MAX];

static int gui_font_load(const char *path)
{
    int i;

    /* Find a previously loaded font. */

    for (i = 0; i < fontc; i++)
        if (strcmp(fonts[i].path, path) == 0)
            return i;

    /* Load a new font. */

    if (fontc < FONT_MAX)
    {
        if (font_load(&fonts[fontc], path, font_sizes))
        {
            fontc++;
            return fontc - 1;
        }
    }

    /* Return index of default font. */

    return 0;
}

static void gui_font_quit(void);

static void gui_font_init(int s)
{
    gui_font_quit();

    if (font_init())
    {
        int i;

        /* Calculate font sizes. */

        for (i = 0; i < FONT_SIZE_MAX; ++i)
            font_sizes[i] = s / font_sizes_scale[i];

        /* Load the default font at index 0. */

#if ENABLE_NLS==1
        int fi = gui_font_load(*curr_lang.font ? curr_lang.font : GUI_FACE);
#else
        int fi = gui_font_load(GUI_FACE);
#endif
    }
    else
        log_errorf("Unable to init font!: %s\n",
                   GAMEDBG_GETSTRERROR_CHOICES_SDL);
}

static void gui_font_quit(void)
{
    int i;

    for (i = 0; i < fontc; i++)
        font_free(&fonts[i]);

    fontc = 0;

    font_quit();
}

/*---------------------------------------------------------------------------*/

static void gui_theme_quit(void)
{
    theme_free(&curr_theme);
}

static void gui_theme_init(void)
{
    gui_theme_quit();
    theme_load(&curr_theme, config_get_s(CONFIG_THEME));
}

/*---------------------------------------------------------------------------*/

static void gui_glyphs_free(void);

static void gui_glyphs_init(void)
{
    int i, j;

    gui_glyphs_free();

    /* Cache digit glyphs for HUD rendering. */

    for (i = 0; i < FONT_SIZE_MAX; i++)
    {
        digit_id[i][0]  = gui_label(0, "0", i, 0, 0);
        digit_id[i][1]  = gui_label(0, "1", i, 0, 0);
        digit_id[i][2]  = gui_label(0, "2", i, 0, 0);
        digit_id[i][3]  = gui_label(0, "3", i, 0, 0);
        digit_id[i][4]  = gui_label(0, "4", i, 0, 0);
        digit_id[i][5]  = gui_label(0, "5", i, 0, 0);
        digit_id[i][6]  = gui_label(0, "6", i, 0, 0);
        digit_id[i][7]  = gui_label(0, "7", i, 0, 0);
        digit_id[i][8]  = gui_label(0, "8", i, 0, 0);
        digit_id[i][9]  = gui_label(0, "9", i, 0, 0);
        digit_id[i][10] = gui_label(0, ":", i, 0, 0);
    }

    for (i = 0; i < FONT_SIZE_MAX; i++)
        for (j = 0; j < 11; ++j)
        {
            gui_set_font(digit_id[i][j], "ttf/DejaVuSans-Bold.ttf");
            gui_layout(digit_id[i][j], 0, 0);
        }

    /* Cache an image for the cursor. Scale it to the same size as a digit. */

#ifdef SWITCHBALL_GUI
    if ((cursor_id = gui_image(0, "gui/cursor.png", ROUND(256.0f * ((float) (video.window_h / 1080.0f))),
                                                    ROUND(256.0f * ((float) (video.window_h / 1080.0f))))))
#else
    if ((cursor_id = gui_image(0, "gui/cursor.png", widget[digit_id[1][0]].w * 2,
                                                    widget[digit_id[1][0]].h * 2)))
#endif
        gui_layout(cursor_id, 0, 0);
}

static void gui_glyphs_free(void)
{
    int i, j;

    for (i = 0; i < FONT_SIZE_MAX; ++i)
        for (j = 0; j < 11; ++j)
        {
            gui_delete(digit_id[i][j]);
            digit_id[i][j] = 0;
        }

    gui_delete(cursor_id);
    cursor_id = 0;
}

/*---------------------------------------------------------------------------*/

static void gui_widget_size(int);

/*
 * Load or reload everything that depends on window/drawable size.
 */
void gui_resize(void)
{
    const int s = MIN(video.device_w, video.device_h);

    int i;

    /* Compute default widget/text padding. */

    padding = s / 60;

    for (i = 0; i < 4; i++)
        borders[i] = padding;

    /* Load font sizes. */

    gui_font_init(s);

    /* Load theme textures. */

    gui_theme_init();

    /* Cache digit glyphs now because some widgets base their size on these. */

    gui_glyphs_init();

    /* Recompute widget space requirements with the new font sizes. */

    for (i = 1; i < WIDGET_MAX; ++i)
        if (widget[i].type != GUI_FREE)
        {
            /*
            * Text widgets get their size from the first rendered string. To that end,
            * we keep that initial string around and use it here to obtain the new text
            * size with the new font size.
            */

            if (widget[i].init_text)
            {
                TTF_Font *ttf = fonts[widget[i].font].ttf[widget[i].size];

                widget[i].text_w = 0;
                widget[i].text_h = 0;

                if (ttf)
                    size_image_from_font(NULL, NULL,
                                         &widget[i].text_w,
                                         &widget[i].text_h,
                                         widget[i].init_text, ttf);
            }

            /* Actually compute the stuff. */

            gui_widget_size(i);
        }

    /* Re-do any saved layouts. Separate from above due to many inter-dependencies. */

    for (i = 1; i < WIDGET_MAX; ++i)
        if (widget[i].type != GUI_FREE && (widget[i].flags & GUI_LAYOUT))
            gui_layout(i, widget[i].layout_xd, widget[i].layout_yd);

}

void gui_init(void)
{
    memset(widget, 0, sizeof (struct widget) * WIDGET_MAX);

    /* Initialize the VBOs. */

    memset(vert_buf, 0, sizeof (vert_buf));

    glGenBuffers_(1,              &vert_vbo);
    glBindBuffer_(GL_ARRAY_BUFFER, vert_vbo);
    glBufferData_(GL_ARRAY_BUFFER, sizeof (vert_buf), vert_buf, GL_STATIC_DRAW);
    glBindBuffer_(GL_ARRAY_BUFFER, 0);

    glGenBuffers_(1,                      &vert_ebo);
    glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER, vert_ebo);
    glBufferData_(GL_ELEMENT_ARRAY_BUFFER,
                  WIDGET_MAX * WIDGET_ELEM * sizeof (GLushort),
                  NULL, GL_STATIC_DRAW);
    glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER, 0);

    /* Initialize window size-dependent resources. */

    gui_resize();

    active = 0;
}

void gui_free(void)
{
    int id;

    /* Release the VBOs. */

    glDeleteBuffers_(1, &vert_vbo);
    glDeleteBuffers_(1, &vert_ebo);

    /* Release any remaining widget texture and display list indices. */

    for (id = 1; id < WIDGET_MAX; id++)
    {
        if (widget[id].type != GUI_FREE)
        {
            if (widget[id].image)
                glDeleteTextures(1, &widget[id].image);

            if (widget[id].init_text)
            {
                free(widget[id].init_text);
                widget[id].init_text = NULL;
            }

            if (widget[id].text)
            {
                free(widget[id].text);
                widget[id].text = NULL;
            }

            widget[id].type  = GUI_FREE;
            widget[id].flags = 0;
            widget[id].image = 0;
            widget[id].cdr   = 0;
            widget[id].car   = 0;
        }
    }

    /* Release all loaded fonts and finalize font rendering. */

    gui_font_quit();

    /* Release theme resources. */

    gui_theme_quit();
}

/*---------------------------------------------------------------------------*/

static int gui_widget(int pd, int type)
{
    int id;

    /* Find an unused entry in the widget table. */

    for (id = 1; id < WIDGET_MAX; id++)
        if (widget[id].type == GUI_FREE)
        {
            /* Set the type and default properties. */

            widget[id].type        = type;
            widget[id].flags       = 0;
            widget[id].hidden      = 0;
            widget[id].token       = 0;
            widget[id].value       = 0;
            widget[id].text        = NULL;
            widget[id].font        = 0;
            widget[id].size        = 0;
            widget[id].rect        = GUI_ALL;
            widget[id].x           = 0;
            widget[id].y           = 0;
            widget[id].w           = 0;
            widget[id].h           = 0;
            widget[id].image       = 0;
            widget[id].color0      = gui_wht;
            widget[id].color1      = gui_wht;
            widget[id].pulse_scale = 1.0f;
            widget[id].trunc       = TRUNC_NONE;
            widget[id].text_w      = 0;
            widget[id].text_h      = 0;

            widget[id].init_text   = NULL;
            widget[id].init_value  = 0;

            widget[id].layout_xd   = 0;
            widget[id].layout_yd   = 0;

            widget[id].offset_init_x = 0.0f;
            widget[id].offset_init_y = 0.0f;
            widget[id].offset_x      = 0.0f;
            widget[id].offset_y      = 0.0f;

            widget[id].slide_flags = 0;
            widget[id].slide_delay = 0.0f;
            widget[id].slide_dur   = 0.0f;
            widget[id].slide_time  = 0.0f;

            widget[id].alpha       = 1.0f;
            widget[id].alpha_slide = 0.0f;

            /* Insert the new widget into the parent's widget list. */

            if (pd)
            {
                widget[id].car = 0;
                widget[id].cdr = widget[pd].car;
                widget[pd].car = id;
            }
            else
            {
                widget[id].car = 0;
                widget[id].cdr = 0;
            }

            return id;
        }

    log_errorf("Out of widget IDs\n");

    return 0;
}

int gui_harray(int pd) { return gui_widget(pd, GUI_HARRAY); }
int gui_varray(int pd) { return gui_widget(pd, GUI_VARRAY); }
int gui_hstack(int pd) { return gui_widget(pd, GUI_HSTACK); }
int gui_vstack(int pd) { return gui_widget(pd, GUI_VSTACK); }
int gui_filler(int pd) { return gui_widget(pd, GUI_FILLER); }

/*
 * For when you really want to use gui_layout on multiple widgets.
 */
int gui_root(void)
{
    int id;

    if ((id = gui_widget(0, GUI_ROOT)))
    {
        /* Get gui_stick() working. */

        widget[id].x = INT_MAX;
        widget[id].y = INT_MAX;
    }
    return id;
}

/*---------------------------------------------------------------------------*/

static struct size gui_measure_ttf(const char *text, TTF_Font *font)
{
    struct size size = { 0, 0 };

    if (text && font)
        TTF_SizeUTF8(font, text, &size.w, &size.h);

    return size;
}

struct size gui_measure(const char *text, int size)
{
    return gui_measure_ttf(text, fonts[0].ttf[size]);
}

/*---------------------------------------------------------------------------*/

static char *gui_trunc_head(const char *text,
                            const int maxwidth,
                            TTF_Font *font)
{
    int left, right, mid;
    char *str = NULL;

    left  = 0;
    right = strlen(text);

    while (right - left > 1)
    {
        mid = (left + right) / 2;

        str = concat_string(GUI_ELLIPSIS, text + mid, NULL);

        if (gui_measure_ttf(str, font).w <= maxwidth)
            right = mid;
        else
            left = mid;

        free(str);
        str = NULL;
    }

    return concat_string(GUI_ELLIPSIS, text + right, NULL);
}

static char *gui_trunc_tail(const char *text,
                            const int maxwidth,
                            TTF_Font *font)
{
    int left, right, mid;
    char *str = NULL;

    left  = 0;
    right = strlen(text);

    while (right - left > 1)
    {
        mid = (left + right) / 2;

        str = malloc(mid + sizeof (GUI_ELLIPSIS));

        memcpy(str,       text,  mid);
        memcpy(str + mid, GUI_ELLIPSIS, sizeof (GUI_ELLIPSIS));

        if (gui_measure_ttf(str, font).w <= maxwidth)
            left = mid;
        else
            right = mid;

        free(str);
        str = NULL;
    }

    str = malloc(left + sizeof (GUI_ELLIPSIS));

    memcpy(str,        text,  left);
    memcpy(str + left, GUI_ELLIPSIS, sizeof (GUI_ELLIPSIS));

    return str;
}

static char *gui_truncate(const char *text,
                          const int maxwidth,
                          TTF_Font *font,
                          enum trunc trunc)
{
    if (gui_measure_ttf(text, font).w <= maxwidth)
        return strdup(text);

    switch (trunc)
    {
        case TRUNC_NONE: return strdup(text);                         break;
        case TRUNC_HEAD: return gui_trunc_head(text, maxwidth, font); break;
        case TRUNC_TAIL: return gui_trunc_tail(text, maxwidth, font); break;
    }

    return NULL;
}

/*---------------------------------------------------------------------------*/

void gui_set_image(int id, const char *file)
{
    FUNC_VOID_CHECK_LIMITS(id);

    glDeleteTextures(1, &widget[id].image);
    gui_img_used = 1;
    widget[id].image = make_image_from_file(file, IF_MIPMAP);
    gui_img_used = 0;
}

void gui_set_label(int id, const char *text)
{
    FUNC_VOID_CHECK_LIMITS(id);

    TTF_Font *ttf = fonts[widget[id].font].ttf[widget[id].size];

    int w = 0;
    int h = 0;

    char *trunc_str, *full_str;

    glDeleteTextures(1, &widget[id].image);

    /* Create a truncated version. */

    trunc_str = gui_truncate(text, widget[id].w, ttf, widget[id].trunc);

    /*
     * Save a copy of the full string in case we need to re-render.
     * The new string COULD BE the exact same old string, so order of
     * operation is important here: copy, free, assign.
     */

    full_str = strdup(text);

    if (widget[id].text)
    {
        free(widget[id].text);
        widget[id].text = NULL;
    }

    widget[id].text = full_str;
    widget[id].text_w = 0;
    widget[id].text_h = 0;

    widget[id].image = make_image_from_font(NULL, NULL,
                                            &widget[id].text_w,
                                            &widget[id].text_h,
                                            trunc_str, ttf, 0);

    /*
     * Rebuild text rectangle.
     *
     * Make sure you have set width and height afterwards before releasing "trunc_str".
     */

    w = widget[id].text_w;
    h = widget[id].text_h;

    gui_geom_text(id, -w / 2, -h / 2, w, h,
                  widget[id].color0 ? widget[id].color0 : gui_wht,
                  widget[id].color1 ? widget[id].color1 : gui_wht);

    free(trunc_str);
    trunc_str = NULL;
}

void gui_set_count(int id, int value)
{
    FUNC_VOID_CHECK_LIMITS(id);

    widget[id].value = value;
}

void gui_set_clock(int id, int value)
{
    FUNC_VOID_CHECK_LIMITS(id);

    widget[id].value = value;
}

void gui_set_color(int id, const GLubyte *c0,
                           const GLubyte *c1)
{
    FUNC_VOID_CHECK_LIMITS(id);

    if (id)
    {
#if NB_HAVE_PB_BOTH==1
        c0 = c0 ? c0 : gui_pnk;
#else
        c0 = c0 ? c0 : gui_yel;
#endif
        c1 = c1 ? c1 : gui_red;

        if (widget[id].color0 != c0 || widget[id].color1 != c1)
        {
            int w = widget[id].text_w;
            int h = widget[id].text_h;

            widget[id].color0 = c0;
            widget[id].color1 = c1;

            gui_geom_text(id, -w / 2, -h / 2, w, h, c0, c1);
        }
    }
}

void gui_set_multi(int id, const char *text)
{
    FUNC_VOID_CHECK_LIMITS(id);

    const char *p;

    char s[GUI_LINES][MAXSTR];
    int i, sc, lc, jd;

    size_t n = 0;

    /* Count available labels. */

    for (lc = 0, jd = widget[id].car; jd; lc++, jd = widget[jd].cdr);

    /* Copy each delimited string to a line buffer. */

    for (p = text, sc = 0; *p && sc < lc; sc++)
    {
        /* Support both '\\' and '\n' as delimiters. */

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        strncpy_s(s[sc], MAXSTR, p, (n = strcspn(p, "\\\n")));
#else
        strncpy(s[sc], p, (n = strcspn(p, "\\\n")));
#endif
        s[sc][n] = 0;

        if (n > 0 && s[sc][n - 1] == '\r')
            s[sc][n - 1] = 0;

        p += n;

        if (*p == '\\' || *p == '\n') p++;
    }

    /* Set the label value for each line. */

    for (i = lc - 1, jd = widget[id].car; i >= 0; i--, jd = widget[jd].cdr)
        gui_set_label(jd, i < sc ? s[i] : "");
}

void gui_set_trunc(int id, enum trunc trunc)
{
    FUNC_VOID_CHECK_LIMITS(id);

    widget[id].trunc = trunc;
}

void gui_set_font(int id, const char *path)
{
    FUNC_VOID_CHECK_LIMITS(id);

    widget[id].font = gui_font_load(path);
}

void gui_set_fill(int id)
{
    FUNC_VOID_CHECK_LIMITS(id);

    widget[id].flags |= GUI_FILL;
}

/*
 * Activate a widget, allowing it  to behave as a normal state widget.
 * This may  be used  to create  image buttons, or  cause an  array of
 * widgets to behave as a single state widget.
 */
int gui_set_state(int id, int token, int value)
{
    FUNC_INT_CHECK_LIMITS(id);

    widget[id].flags |= GUI_STATE;
    widget[id].token  = token;
    widget[id].value  = value;

    return id;
}

void gui_set_hilite(int id, int hilite)
{
    FUNC_VOID_CHECK_LIMITS(id);

    if (hilite)
        widget[id].flags |= GUI_HILITE;
    else
        widget[id].flags &= ~GUI_HILITE;
}

void gui_set_rect(int id, int rect)
{
    FUNC_VOID_CHECK_LIMITS(id);

    widget[id].rect   = rect;
    widget[id].flags |= GUI_RECT;
}

void gui_clr_rect(int id)
{
    FUNC_VOID_CHECK_LIMITS(id);

    int jd;

    widget[id].flags &= ~GUI_RECT;

    for (jd = widget[id].car; jd; jd = widget[jd].cdr)
        gui_clr_rect(jd);
}

void gui_set_clip(int id)
{
    FUNC_VOID_CHECK_LIMITS(id);

    widget[id].flags |= GUI_CLIP;
}

void gui_clr_clip(int id)
{
    FUNC_VOID_CHECK_LIMITS(id);

    int jd;

    widget[id].flags &= ~GUI_CLIP;

    for (jd = widget[id].car; jd; jd = widget[jd].cdr)
        gui_clr_clip(jd);
}

void gui_set_cursor(int st)
{
    cursor_st = st;
}

void gui_set_hidden(int id, int hidden)
{
    FUNC_VOID_CHECK_LIMITS(id);

    if (id)
        widget[id].hidden = hidden ? 1u : 0;
}

/*---------------------------------------------------------------------------*/

/*
 * Compute widget size requirements.
 */
static void gui_widget_size(int id)
{
    int i;

    const int s = MIN(video.device_w, video.device_h);

    switch (widget[id].type)
    {
        case GUI_IMAGE:
            /* Convert from integer-encoded fractions to window pixels. */

            widget[id].w = ROUND(((float) widget[id].text_w / 1000.0f) * (float) s);
            widget[id].h = ROUND(((float) widget[id].text_h / 1000.0f) * (float) s);

            break;

        case GUI_BUTTON:
        case GUI_LABEL:
            widget[id].w = widget[id].text_w;
            widget[id].h = widget[id].text_h;
            break;

        case GUI_COUNT:
            widget[id].w = 0;

            for (i = widget[id].init_value; i; i /= 10)
                widget[id].w += widget[digit_id[widget[id].size][0]].text_w;

            widget[id].h = widget[digit_id[widget[id].size][0]].text_h;

            break;

        case GUI_CLOCK:
            widget[id].w = widget[digit_id[widget[id].size][0]].text_w * 6;
            widget[id].h = widget[digit_id[widget[id].size][0]].text_h;
            break;

        default:
            widget[id].w = 0;
            widget[id].h = 0;
            break;
    }
}

int gui_image(int pd, const char *file, int w, int h)
{
    int id;

    const int s = MIN(video.device_w, video.device_h);

    if ((id = gui_widget(pd, GUI_IMAGE)))
    {
        gui_img_used = 1;
        widget[id].image  = make_image_from_file(file, IF_MIPMAP);

        /* Convert window pixels to integer-encoded fractions. */

        widget[id].text_w  = ROUND(((float) w / (float) s) * 1000.0f);
        widget[id].text_h  = ROUND(((float) h / (float) s) * 1000.0f);
        widget[id].flags  |= GUI_RECT;

        gui_widget_size(id);
        gui_img_used = 0;
    }
    return id;
}

int gui_start(int pd, const char *text, int size, int token, int value)
{
    int id;

    if ((id = gui_state(pd, text, size, token, value)))
        active = id;

    return id;
}

int gui_state(int pd, const char *text, int size, int token, int value)
{
    int id;

    if ((id = gui_widget(pd, GUI_BUTTON)))
    {
        TTF_Font *ttf = fonts[widget[id].font].ttf[size];

        widget[id].flags |= (GUI_STATE | GUI_RECT);

        widget[id].init_text = strdup(text);

        widget[id].text = strdup(text);

        size_image_from_font(NULL, NULL,
                             &widget[id].text_w,
                             &widget[id].text_h,
                             text, ttf);

        widget[id].size  = size;
        widget[id].token = token;
        widget[id].value = value;
        gui_widget_size(id);
    }
    return id;
}

int gui_label(int pd, const char *text, int size, const GLubyte *c0,
                                                  const GLubyte *c1)
{
    int id;

    if ((id = gui_widget(pd, GUI_LABEL)))
    {
        TTF_Font *ttf;
        memset(&ttf, 0, sizeof (TTF_Font *));
        ttf = fonts[widget[id].font].ttf[size];
#ifndef NDEBUG
        assert(&ttf);
#endif

        widget[id].init_text = strdup(text);

        widget[id].text = strdup(text);

        size_image_from_font(NULL, NULL,
                             &widget[id].text_w,
                             &widget[id].text_h,
                             text, ttf);
        widget[id].w      = widget[id].text_w;
        widget[id].h      = widget[id].text_h;
        widget[id].size   = size;
#if NB_HAVE_PB_BOTH==1
        widget[id].color0 = c0 ? c0 : gui_pnk;
#else
        widget[id].color0 = c0 ? c0 : gui_yel;
#endif
        widget[id].color1 = c1 ? c1 : gui_red;
        widget[id].flags |= GUI_RECT;
        gui_widget_size(id);
    }
    return id;
}

int gui_title_header(int pd, const char *text, int size, const GLubyte *c0,
                                                         const GLubyte *c1)
{
    int i = size;
    int width_fit = 0;

    while (i > 0 && width_fit == 0)
    {
        int max_width = gui_measure(text, i).w;

        width_fit = max_width < (video.device_w * .85f) ? 1 : 0;

        if (!width_fit) i--;
    }

    return gui_label(pd, text, i, c0, c1);
}

int gui_count(int pd, int value, int size)
{
    int id;

    if ((id = gui_widget(pd, GUI_COUNT)))
    {
        widget[id].init_value = value;
        widget[id].value      = value;
        widget[id].size       = size;
#if NB_HAVE_PB_BOTH==1
        widget[id].color0     = gui_pnk;
#else
        widget[id].color0     = gui_yel;
#endif
        widget[id].color1     = gui_red;
        widget[id].flags     |= GUI_RECT;

        gui_widget_size(id);
    }
    return id;
}

int gui_clock(int pd, int value, int size)
{
    int id;

    if ((id = gui_widget(pd, GUI_CLOCK)))
    {
        widget[id].init_value = value;
        widget[id].value      = value;
        widget[id].size       = size;
#if NB_HAVE_PB_BOTH==1
        widget[id].color0     = gui_pnk;
#else
        widget[id].color0     = gui_yel;
#endif
        widget[id].color1     = gui_red;
        widget[id].flags     |= GUI_RECT;
        gui_widget_size(id);
    }
    return id;
}

int gui_space(int pd)
{
    return gui_widget(pd, GUI_SPACE);;
}

/*---------------------------------------------------------------------------*/

/*
 * Create  a multi-line  text box  using a  vertical array  of labels.
 * Parse the  text for '\'  characters and treat them  as line-breaks.
 * Preserve the rect specification across the entire array.
 */

int gui_multi(int pd, const char *text, int size, const GLubyte *c0,
                                                  const GLubyte *c1)
{
    int id = 0;

    if (text && *text && (id = gui_varray(pd)))
    {
        const char *p;

        char s[GUI_LINES][MAXSTR];
        int  i, j;

        size_t n = 0;

        /* Copy each delimited string to a line buffer. */

        for (p = text, j = 0; *p && j < GUI_LINES; j++)
        {
            /* Support both '\\' and '\n' as delimiters. */

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            strncpy_s(s[j], MAXSTR, p, (n = strcspn(p, "\\\n")));
#else
            strncpy(s[j], p, (n = strcspn(p, "\\\n")));
#endif
            s[j][n] = 0;

            if (n > 0 && s[j][n - 1] == '\r')
                s[j][n - 1] = 0;

            p += n;

            if (*p == '\\' || *p == '\n') p++;
        }

        /* Create a label widget for each line. */

        for (i = 0; i < j; i++)
            gui_label(id, s[i], size, c0, c1);

        /* Set rectangle on the container. */

        widget[id].flags |= GUI_RECT;
        widget[id].flags |= GUI_CLIP;
    }
    return id;
}

/*---------------------------------------------------------------------------*/
/*
 * The bottom-up pass determines the area of all widgets.  The minimum
 * width  and height of  a leaf  widget is  given by  the size  of its
 * contents.   Array  and  stack   widths  and  heights  are  computed
 * recursively from these.
 */

static void gui_widget_up(int id);

static void gui_harray_up(int id)
{
    int jd, c = 0;

    /* Find the widest child width and the highest child height. */

    for (jd = widget[id].car; jd; jd = widget[jd].cdr)
    {
        gui_widget_up(jd);

        if (widget[id].h < widget[jd].h)
            widget[id].h = widget[jd].h;
        if (widget[id].w < widget[jd].w)
            widget[id].w = widget[jd].w;

        c++;
    }

    /* Total width is the widest child width times the child count. */

    widget[id].w *= c;
}

static void gui_varray_up(int id)
{
    int jd, c = 0;

    /* Find the widest child width and the highest child height. */

    for (jd = widget[id].car; jd; jd = widget[jd].cdr)
    {
        gui_widget_up(jd);

        if (widget[id].h < widget[jd].h)
            widget[id].h = widget[jd].h;
        if (widget[id].w < widget[jd].w)
            widget[id].w = widget[jd].w;

        c++;
    }

    /* Total height is the highest child height times the child count. */

    widget[id].h *= c;
}

static void gui_hstack_up(int id)
{
    int jd;

    /* Find the highest child height.  Sum the child widths. */

    for (jd = widget[id].car; jd; jd = widget[jd].cdr)
    {
        gui_widget_up(jd);

        if (widget[id].h < widget[jd].h)
            widget[id].h = widget[jd].h;

        widget[id].w += widget[jd].w;
    }
}

static void gui_vstack_up(int id)
{
    int jd;

    /* Find the widest child width.  Sum the child heights. */

    for (jd = widget[id].car; jd; jd = widget[jd].cdr)
    {
        gui_widget_up(jd);

        if (widget[id].w < widget[jd].w)
            widget[id].w = widget[jd].w;

        widget[id].h += widget[jd].h;
    }
}

static void gui_button_up(int id)
{
    if (widget[id].w < widget[id].h && widget[id].w > 0)
        widget[id].w = widget[id].h;

    /* Padded text elements look a little nicer. */

    if (widget[id].w < video.device_w)
        widget[id].w += padding;
    if (widget[id].h < video.device_h)
        widget[id].h += padding;

    /* A button should be at least wide enough to accomodate the borders. */

    if (widget[id].w < borders[0] + borders[1])
        widget[id].w = borders[0] + borders[1];
    if (widget[id].h < borders[2] + borders[3])
        widget[id].h = borders[2] + borders[3];
}

static void gui_widget_up(int id)
{
    if (id)
        switch (widget[id].type)
        {
            case GUI_HARRAY: gui_harray_up(id); break;
            case GUI_VARRAY: gui_varray_up(id); break;
            case GUI_HSTACK: gui_hstack_up(id); break;
            case GUI_VSTACK: gui_vstack_up(id); break;
            case GUI_FILLER:                    break;
            default:         gui_button_up(id); break;
        }
}

/*---------------------------------------------------------------------------*/
/*
 * The  top-down layout  pass distributes  available area  as computed
 * during the bottom-up pass.  Widgets  use their area and position to
 * initialize rendering state.
 */

static void gui_widget_dn(int id, int x, int y, int w, int h);

static void gui_harray_dn(int id, int x, int y, int w, int h)
{
    int jd, i = 0, c = 0;

    widget[id].x = x;
    widget[id].y = y;
    widget[id].w = w;
    widget[id].h = h;

    /* Count children. */

    for (jd = widget[id].car; jd; jd = widget[jd].cdr)
        c += 1;

    /* Distribute horizontal space evenly to all children. */

    for (jd = widget[id].car; jd; jd = widget[jd].cdr, i++)
    {
        int x0 = x +  i      * w / c;
        int x1 = x + (i + 1) * w / c;

        gui_widget_dn(jd, x0, y, x1 - x0, h);
    }
}

static void gui_varray_dn(int id, int x, int y, int w, int h)
{
    int jd, i = 0, c = 0;

    widget[id].x = x;
    widget[id].y = y;
    widget[id].w = w;
    widget[id].h = h;

    /* Count children. */

    for (jd = widget[id].car; jd; jd = widget[jd].cdr)
        c += 1;

    /* Distribute vertical space evenly to all children. */

    for (jd = widget[id].car; jd; jd = widget[jd].cdr, i++)
    {
        int y0 = y +  i      * h / c;
        int y1 = y + (i + 1) * h / c;

        gui_widget_dn(jd, x, y0, w, y1 - y0);
    }
}

static void gui_hstack_dn(int id, int x, int y, int w, int h)
{
    int jd, jx = x, jw = 0, dw = 0, c = 0;

    widget[id].x = x;
    widget[id].y = y;
    widget[id].w = w;
    widget[id].h = h;

    /* Measure the total width requested by non-filler children. */

    for (jd = widget[id].car; jd; jd = widget[jd].cdr)
    {
        if (widget[jd].type == GUI_FILLER)
            c += 1;
        else if (widget[jd].flags & GUI_FILL)
        {
            c += 1;
            jw += widget[jd].w;
        }
        else
            jw += widget[jd].w;
    }

    /* Give non-filler children their requested space.   */
    /* Distribute the rest evenly among filler children. */

    dw = c > 0 ? (w - jw) / c : 0;

    for (jd = widget[id].car; jd; jd = widget[jd].cdr)
    {
        if (widget[jd].type == GUI_FILLER)
            gui_widget_dn(jd, jx, y, dw, h);
        else if (widget[jd].flags & GUI_FILL)
            gui_widget_dn(jd, jx, y, widget[jd].w + dw, h);
        else
            gui_widget_dn(jd, jx, y, widget[jd].w, h);

        jx += widget[jd].w;
    }
}

static void gui_vstack_dn(int id, int x, int y, int w, int h)
{
    int jd, jy = y, jh = 0, c = 0;

    widget[id].x = x;
    widget[id].y = y;
    widget[id].w = w;
    widget[id].h = h;

    /* Measure the total height requested by non-filler children. */

    for (jd = widget[id].car; jd; jd = widget[jd].cdr)
        if (widget[jd].type == GUI_FILLER)
            c += 1;
        else if (widget[jd].flags & GUI_FILL)
        {
            c  += 1;
            jh += widget[jd].h;
        }
        else
            jh += widget[jd].h;

    /* Give non-filler children their requested space.   */
    /* Distribute the rest evenly among filler children. */

    for (jd = widget[id].car; jd; jd = widget[jd].cdr)
    {
        if (widget[jd].type == GUI_FILLER)
            gui_widget_dn(jd, x, jy, w, (h - jh) / c);
        else if (widget[jd].flags & GUI_FILL)
            gui_widget_dn(jd, x, jy, w, widget[jd].h + (h - jh) / c);
        else
            gui_widget_dn(jd, x, jy, w, widget[jd].h);

        jy += widget[jd].h;
    }
}

static void gui_filler_dn(int id, int x, int y, int w, int h)
{
    /* Filler expands to whatever size it is given. */

    widget[id].x = x;
    widget[id].y = y;
    widget[id].w = w;
    widget[id].h = h;
}

static void gui_button_dn(int id, int x, int y, int w, int h)
{
    widget[id].x = x;
    widget[id].y = y;
    widget[id].w = w;
    widget[id].h = h;
}

static void gui_widget_dn(int id, int x, int y, int w, int h)
{
    if (id)
        switch (widget[id].type)
        {
            case GUI_HARRAY: gui_harray_dn(id, x, y, w, h); break;
            case GUI_VARRAY: gui_varray_dn(id, x, y, w, h); break;
            case GUI_HSTACK: gui_hstack_dn(id, x, y, w, h); break;
            case GUI_VSTACK: gui_vstack_dn(id, x, y, w, h); break;
            case GUI_FILLER: gui_filler_dn(id, x, y, w, h); break;
            case GUI_SPACE:  gui_filler_dn(id, x, y, w, h); break;
            default:         gui_button_dn(id, x, y, w, h); break;
        }
}

/*---------------------------------------------------------------------------*/

static void gui_widget_offset(int id, int pd)
{
    FUNC_VOID_CHECK_LIMITS(id);

    if (id)
    {
        int jd;

        if (!(widget[id].flags & GUI_OFFSET))
        {
            for (jd = widget[id].car; jd; jd = widget[jd].cdr)
                gui_widget_offset(jd, 0);

            return;
        }

        /* HACK: restart animations in case we got here via resize event. */
        widget[id].slide_time = 0.0f;

        widget[id].offset_init_x = widget[id].offset_x = 0.0f;
        widget[id].offset_init_y = widget[id].offset_y = 0.0f;

        if (widget[id].slide_flags & GUI_FLING)
        {
            // Offset position is a full frame away.

            if (widget[id].slide_flags & GUI_W)
                widget[id].offset_init_x = (float) -video.device_w;

            if (widget[id].slide_flags & GUI_E)
                widget[id].offset_init_x = (float) +video.device_w;

            if (widget[id].slide_flags & GUI_S)
                widget[id].offset_init_y = (float) -video.device_h;

            if (widget[id].slide_flags & GUI_N)
                widget[id].offset_init_y = (float) +video.device_h;
        }
        else
        {
            // Offset position is just offscreen.

            if (pd)
            {
                // If parent is also offset, inherit.

                if (widget[id].slide_flags & GUI_W)
                    widget[id].offset_init_x = widget[pd].offset_init_x;

                if (widget[id].slide_flags & GUI_E)
                    widget[id].offset_init_x = widget[pd].offset_init_x;

                if (widget[id].slide_flags & GUI_S)
                    widget[id].offset_init_y = widget[pd].offset_init_y;

                if (widget[id].slide_flags & GUI_N)
                    widget[id].offset_init_y = widget[pd].offset_init_y;
            }
            else
            {
                if (widget[id].slide_flags & GUI_W)
                    widget[id].offset_init_x = -widget[id].x - widget[id].w;

                if (widget[id].slide_flags & GUI_E)
                    widget[id].offset_init_x = video.device_w - widget[id].x + 1;

                if (widget[id].slide_flags & GUI_S)
                    widget[id].offset_init_y = -widget[id].y - widget[id].h - 1;

                if (widget[id].slide_flags & GUI_N)
                    widget[id].offset_init_y = video.device_h - widget[id].y;
            }
        }

        if (!(widget[id].slide_flags & GUI_BACKWARD))
        {
            widget[id].offset_x = widget[id].offset_init_x;
            widget[id].offset_y = widget[id].offset_init_y;
        }

        for (jd = widget[id].car; jd; jd = widget[jd].cdr)
            gui_widget_offset(jd, id);
    }
}

void gui_set_slide(int id, int flags, float delay, float t, float stagger)
{
    if (!config_get_d(CONFIG_SCREEN_ANIMATIONS)) return;

    FUNC_VOID_CHECK_LIMITS(id);

    if (id)
    {
        int jd, c = 0;

        widget[id].flags |= GUI_OFFSET;

        widget[id].slide_flags = flags;
        widget[id].slide_delay = delay;
        widget[id].slide_dur = t;
        widget[id].slide_time = 0.0f;

        for (jd = widget[id].car; jd; jd = widget[jd].cdr)
            c++;

        for (jd = widget[id].car; jd; jd = widget[jd].cdr, c--)
            gui_set_slide(jd, flags, delay + (stagger * (c - 1)), t, 0.0f);
    }
}

void gui_slide(int id, int flags, float delay, float t, float stagger)
{
    FUNC_VOID_CHECK_LIMITS(id);

    if (id)
    {
        gui_set_slide(id, flags, delay, t, stagger);
        gui_widget_offset(id, 0);
    }
}

/*---------------------------------------------------------------------------*/

static void gui_render_text(int id)
{
    int jd;

    if (widget[id].type != GUI_FREE && widget[id].text)
        gui_set_label(id, widget[id].text);

    for (jd = widget[id].car; jd; jd = widget[jd].cdr)
        gui_render_text(jd);
}

/*
 * During GUI layout, we make a bottom-up pass to determine total area
 * requirements for  the widget  tree.  We position  this area  to the
 * sides or center of the screen.  Finally, we make a top-down pass to
 * distribute this area to each widget.
 */

void gui_layout(int id, int xd, int yd)
{
    int x, y;

    int w, W = video.device_w;
    int h, H = video.device_h;

    widget[id].flags     |= GUI_LAYOUT;
    widget[id].layout_xd  = xd;
    widget[id].layout_yd  = yd;

    gui_widget_up(id);

    w = MIN(widget[id].w, W - padding * 2);
    h = MIN(widget[id].h, H - padding * 2);

    if      (xd < 0) x = 0;
    else if (xd > 0) x = (W - w);
    else             x = (W - w) / 2;

    if      (yd < 0) y = 0;
    else if (yd > 0) y = (H - h);
    else             y = (H - h) / 2;

    gui_widget_dn(id, x, y, w, h);

    /* Initialize animation state. */

    gui_widget_offset(id, 0);

    /* Set up GUI rendering state. */

    gui_geom_widget(id, 0);

    gui_render_text(id);

    /* Hilite the widget under the cursor, if any. */

    gui_point(id, -1, -1);
}

int gui_search(int id, int x, int y)
{
    FUNC_INT_CHECK_LIMITS(id);

    int jd, kd;

    /* Search the hierarchy for the widget containing the given point. */

    const int offset_total_x = widget[id].x + ROUND(widget[id].offset_x);
    const int offset_total_y = widget[id].y + ROUND(widget[id].offset_y);

    if (id &&
        (widget[id].type == GUI_ROOT ||
         (offset_total_x <= x && x < offset_total_x + widget[id].w &&
          offset_total_y <= y && y < offset_total_y + widget[id].h)))
    {
        if (gui_hot(id))
            return id;

        for (jd = widget[id].car; jd; jd = widget[jd].cdr)
            if ((kd = gui_search(jd, x, y)))
                return kd;
    }
    return 0;
}

int gui_child(int id, int index)
{
    FUNC_INT_CHECK_LIMITS(id);

    if (id)
    {
        int jd, c = 0, i;

        for (jd = widget[id].car; jd; jd = widget[jd].cdr)
            c++;

        for (jd = widget[id].car, i = c - 1; jd; jd = widget[jd].cdr, i--)
            if (i == index)
                return jd;
    }

    return 0;
}

int gui_delete(int id)
{
    FUNC_INT_CHECK_LIMITS(id);

    if (id && widget[id].type != GUI_FREE)
    {
        /* Recursively delete all subwidgets. */

        gui_delete(widget[id].cdr);
        gui_delete(widget[id].car);

        if (widget[id].text)
        {
            free(widget[id].text);
            widget[id].text = NULL;
        }

        if (widget[id].init_text)
        {
            free(widget[id].init_text);
            widget[id].init_text = NULL;
        }

        /* Release any GL resources held by this widget. */

        if (widget[id].image)
            glDeleteTextures(1, &widget[id].image);

        /* Mark this widget unused. */

        widget[id].type  = GUI_FREE;
        widget[id].flags = 0;
        widget[id].image = 0;
        widget[id].cdr   = 0;
        widget[id].car   = 0;

        /* Clear focus from this widget. */

        if (active == id)
            active = 0;
    }
    return 0;
}

/*
 * Delete this widget and update the layout of its parent widget.
 */
void gui_remove(int id)
{
    FUNC_VOID_CHECK_LIMITS(id);

    if (id && widget[id].type != GUI_FREE)
    {
        int pd = 0;
        int i;

        for (i = 1; i < WIDGET_MAX; ++i)
            if (widget[i].type != GUI_FREE)
            {
                int jd, pjd;

                for (pjd = 0, jd = widget[i].car; jd; pjd = jd, jd = widget[jd].cdr)
                    if (jd == id)
                    {
                        /* Note the parent widget. */

                        pd = i;

                        /* Remove from linked list. */

                        if (pjd)
                            widget[pjd].cdr = widget[jd].cdr; /* prev = next */
                        else
                            widget[i].car = widget[jd].cdr; /* head = next */
                    }
            }

        /* Update parent widget layout. */

        if (pd)
        {
            gui_widget_dn(pd, widget[pd].x, widget[pd].y, widget[pd].w, widget[pd].h);
            gui_geom_widget(pd, widget[pd].flags);
        }

        gui_delete(id);

        transition_remove(id);
    }
}

/*---------------------------------------------------------------------------*/

static void gui_paint_rect(int id, int st, int flags)
{
    int jd, i = 0;

    if (widget[id].hidden)
        return;

    /* Use the widget status to determine the background color. */

    i = st | (((widget[id].flags & GUI_HILITE) ? 2 : 0) |
              ((id == active)                  ? 1 : 0));

    if ((widget[id].flags & GUI_RECT) && !(flags & GUI_RECT))
    {
        /* Draw a leaf's background, colored by widget state. */

        glPushMatrix();
        {
            glTranslatef((GLfloat) (widget[id].x + widget[id].w / 2 + widget[id].offset_x),
                         (GLfloat) (widget[id].y + widget[id].h / 2 + widget[id].offset_y), 0.0f);

            glBindTexture_(GL_TEXTURE_2D, curr_theme.tex[i]);
            draw_rect(id);
        }
        glPopMatrix();

        flags |= GUI_RECT;
    }

    switch (widget[id].type)
    {
        case GUI_HARRAY:
        case GUI_VARRAY:
        case GUI_HSTACK:
        case GUI_VSTACK:
        case GUI_ROOT:
            /* Recursively paint all subwidgets. */

            for (jd = widget[id].car; jd; jd = widget[jd].cdr)
                gui_paint_rect(jd, i, flags);

            break;
    }
}

/*---------------------------------------------------------------------------*/

static void gui_paint_text(int id);

static void gui_paint_array(int id)
{
    int jd;

    glPushMatrix();
    {
        GLfloat cx = widget[id].x + widget[id].w / 2.0f + widget[id].offset_x;
        GLfloat cy = widget[id].y + widget[id].h / 2.0f + widget[id].offset_y;
        GLfloat ck = widget[id].pulse_scale;

        if (1.0f < ck || ck < 1.0f)
        {
            glTranslatef(+cx, +cy, 0.0f);
            glScalef(ck, ck, ck);
            glTranslatef(-cx, -cy, 0.0f);
        }

        /* Recursively paint all subwidgets. */

#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
        if (widget[id].flags & GUI_CLIP)
        {
            glScissor(
                widget[id].x + widget[id].offset_x,
                widget[id].y + widget[id].offset_y,
                widget[id].w,
                widget[id].h
            );
            glEnable(GL_SCISSOR_TEST);
        }
#endif

        for (jd = widget[id].car; jd; jd = widget[jd].cdr)
            gui_paint_text(jd);

#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
        if (widget[id].flags & GUI_CLIP)
            glDisable(GL_SCISSOR_TEST);
#endif
    }
    glPopMatrix();
}

static void gui_paint_image(int id)
{
    if (widget[id].hidden)
        return;

    /* Draw the widget rect, textured using the image. */

    glPushMatrix();
    {
        glTranslatef((GLfloat) (widget[id].x + widget[id].w / 2 + widget[id].offset_x),
                     (GLfloat) (widget[id].y + widget[id].h / 2 + widget[id].offset_y), 0.0f);

        glScalef(widget[id].pulse_scale,
                 widget[id].pulse_scale,
                 widget[id].pulse_scale);

        glBindTexture_(GL_TEXTURE_2D, widget[id].image);
        glColor4ub(GUI_COLOR4UB);
        draw_image(id);
    }
    glPopMatrix();
}

static void gui_paint_count(int id)
{
    if (widget[id].hidden)
        return;

    int j, i = widget[id].size;

    glPushMatrix();
    {
        /* Translate to the widget center, and apply the pulse scale. */

        glTranslatef((GLfloat) (widget[id].x + widget[id].w / 2 + widget[id].offset_x),
                     (GLfloat) (widget[id].y + widget[id].h / 2 + widget[id].offset_y), 0.0f);

        glScalef(widget[id].pulse_scale,
                 widget[id].pulse_scale,
                 widget[id].pulse_scale);

        if (widget[id].value > 0)
        {
            /* Translate right by half the total width of the rendered value. */

            GLfloat w = -widget[digit_id[i][0]].text_w * 0.5f;

            for (j = widget[id].value; j; j /= 10)
                w += widget[digit_id[i][j % 10]].text_w * 0.5f;

            glTranslatef(w, 0.0f, 0.0f);

            /* Render each digit, moving left after each. */

            for (j = widget[id].value; j; j /= 10)
            {
                int jd = digit_id[i][j % 10];

                glBindTexture_(GL_TEXTURE_2D, widget[jd].image);
                glColor4ub(GUI_COLOR4UB);
                draw_text(jd);
                glTranslatef((GLfloat) -widget[jd].text_w, 0.0f, 0.0f);
            }
        }
        else if (widget[id].value == 0)
        {
            /* If the value is zero, just display a zero in place. */

            glBindTexture_(GL_TEXTURE_2D, widget[digit_id[i][0]].image);
            glColor4ub(GUI_COLOR4UB);
            draw_text(digit_id[i][0]);
        }
    }
    glPopMatrix();
}

static void gui_paint_clock(int id)
{
    if (widget[id].hidden)
        return;

    int i   =   widget[id].size;

    if (widget[id].value < 0)
        return;

    /*int dyh = ((widget[id].value / 864000000) / 100) % 100;
    int dyt = ((widget[id].value / 864000000) / 10) % 10;
    int dyo =  (widget[id].value / 864000000) % 10;*/

    int temp_value = widget[id].value;

    /* Overlength above 24 hours. */

    while (((temp_value / 360000) / 100) >= 24)
        temp_value -= 864000000;

    int hrt =  (temp_value / 360000) / 1000;
    int hro = ((temp_value / 360000) / 100) % 10;
    int mt  = ((temp_value / 6000) / 10) % 6;
    int mo  =  (temp_value / 6000) % 10;
    int st  = ((temp_value % 6000) / 100) / 10;
    int so  = ((temp_value % 6000) / 100) % 10;
    int ht  = ((temp_value % 6000) % 100) / 10;
    int ho  = ((temp_value % 6000) % 100) % 10;

    GLfloat dx_large = (GLfloat) widget[digit_id[i][0]].text_w;
    GLfloat dx_small = (GLfloat) widget[digit_id[i][0]].text_w * 0.75f;

    glPushMatrix();
    {
        /* Translate to the widget center, and apply the pulse scale. */

        glTranslatef((GLfloat) (widget[id].x + widget[id].w / 2 + widget[id].offset_x),
                     (GLfloat) (widget[id].y + widget[id].h / 2 + widget[id].offset_y), 0.0f);

        glScalef(widget[id].pulse_scale,
                 widget[id].pulse_scale,
                 widget[id].pulse_scale);

        /* Translate left by half the total width of the rendered value. */

        /*if (dyh > 0)
            glTranslatef(-5.25f * dx_large, 0.0f, 0.0f);
        else if (dyt > 0)
            glTranslatef(-4.75f * dx_large, 0.0f, 0.0f);
        else if (dyo > 0)
            glTranslatef(-4.25f * dx_large, 0.0f, 0.0f);
        else*/ if (hrt > 0)
            glTranslatef(-3.75f * dx_large, 0.0f, 0.0f);
        else if (hro > 0)
            glTranslatef(-3.25f * dx_large, 0.0f, 0.0f);
        else if (mt > 0)
            glTranslatef(-2.25f * dx_large, 0.0f, 0.0f);
        else
            glTranslatef(-1.75f * dx_large, 0.0f, 0.0f);

        /* Render the day counter. */

        /*if (dyh > 0)
        {
            glBindTexture_(GL_TEXTURE_2D, widget[digit_id[i][dyh]].image);
            glColor4ub(GUI_COLOR4UB);
            draw_text(digit_id[i][dyh]);
            glTranslatef(dx_large, 0.0f, 0.0f);
        }

        if (dyt > 0 || dyh > 0)
        {
            glBindTexture_(GL_TEXTURE_2D, widget[digit_id[i][dyt]].image);
            glColor4ub(GUI_COLOR4UB);
            draw_text(digit_id[i][dyt]);
            glTranslatef(dx_large, 0.0f, 0.0f);
        }

        if (dyo > 0 || dyt > 0 || dyh > 0)
        {
            glBindTexture_(GL_TEXTURE_2D, widget[digit_id[i][dyo]].image);
            glColor4ub(GUI_COLOR4UB);
            draw_text(digit_id[i][dyo]);
            glTranslatef(dx_small, 0.0f, 0.0f);
        }*/

        /* Render the hyphen (before hours). */

        /*if (dyo > 0 || dyt > 0 || dyh > 0)
        {
            glBindTexture_(GL_TEXTURE_2D, widget[digit_id[i][10]].image);
            glColor4ub(GUI_COLOR4UB);
            draw_text(digit_id[i][11]);
            glTranslatef(dx_small, 0.0f, 0.0f);
        }*/

        /* Render the hour counter. */

        if (hrt > 0)
        {
            glBindTexture_(GL_TEXTURE_2D, widget[digit_id[i][hrt]].image);
            glColor4ub(GUI_COLOR4UB);
            draw_text(digit_id[i][hrt]);
            glTranslatef(dx_large, 0.0f, 0.0f);
        }

        if (hro > 0 || hrt > 0)
        {
            glBindTexture_(GL_TEXTURE_2D, widget[digit_id[i][hro]].image);
            glColor4ub(GUI_COLOR4UB);
            draw_text(digit_id[i][hro]);
            glTranslatef(dx_small, 0.0f, 0.0f);
        }

        /* Render the colon (before minutes). */

        if (hro > 0 || hrt > 0)
        {
            glBindTexture_(GL_TEXTURE_2D, widget[digit_id[i][10]].image);
            glColor4ub(GUI_COLOR4UB);
            draw_text(digit_id[i][10]);
            glTranslatef(dx_small, 0.0f, 0.0f);
        }

        /* Render the minutes counter. */

        if (mt > 0 || hro > 0)
        {
            glBindTexture_(GL_TEXTURE_2D, widget[digit_id[i][mt]].image);
            glColor4ub(GUI_COLOR4UB);
            draw_text(digit_id[i][mt]);
            glTranslatef(dx_large, 0.0f, 0.0f);
        }

        glBindTexture_(GL_TEXTURE_2D, widget[digit_id[i][mo]].image);
        glColor4ub(GUI_COLOR4UB);
        draw_text(digit_id[i][mo]);
        glTranslatef(dx_small, 0.0f, 0.0f);

        /* Render the colon (before seconds). */

        glBindTexture_(GL_TEXTURE_2D, widget[digit_id[i][10]].image);
        glColor4ub(GUI_COLOR4UB);
        draw_text(digit_id[i][10]);
        glTranslatef(dx_small, 0.0f, 0.0f);

        /* Render the seconds counter. */

        glBindTexture_(GL_TEXTURE_2D, widget[digit_id[i][st]].image);
        glColor4ub(GUI_COLOR4UB);
        draw_text(digit_id[i][st]);
        glTranslatef(dx_large, 0.0f, 0.0f);

        glBindTexture_(GL_TEXTURE_2D, widget[digit_id[i][so]].image);
        glColor4ub(GUI_COLOR4UB);
        draw_text(digit_id[i][so]);
        glTranslatef(dx_small, 0.0f, 0.0f);

        /* Render hundredths counter half size. */

        glScalef(0.5f, 0.5f, 1.0f);

        glBindTexture_(GL_TEXTURE_2D, widget[digit_id[i][ht]].image);
        glColor4ub(GUI_COLOR4UB);
        draw_text(digit_id[i][ht]);
        glTranslatef(dx_large, 0.0f, 0.0f);

        glBindTexture_(GL_TEXTURE_2D, widget[digit_id[i][ho]].image);
        glColor4ub(GUI_COLOR4UB);
        draw_text(digit_id[i][ho]);
    }
    glPopMatrix();
}

static void gui_paint_label(int id)
{
    /* Short-circuit empty labels. */

    if (widget[id].image == 0 || widget[id].hidden)
        return;

    /* Draw the widget text box, textured using the glyph. */

    glPushMatrix();
    {
        glTranslatef((GLfloat) (widget[id].x + widget[id].w / 2 + widget[id].offset_x),
                     (GLfloat) (widget[id].y + widget[id].h / 2 + widget[id].offset_y), 0.0f);

        glScalef(widget[id].pulse_scale,
                 widget[id].pulse_scale,
                 widget[id].pulse_scale);

        glBindTexture_(GL_TEXTURE_2D, widget[id].image);
        glColor4ub(GUI_COLOR4UB);
        draw_text(id);
    }
    glPopMatrix();
}

static void gui_paint_text(int id)
{
    if (widget[id].hidden || (widget[id].alpha_slide * widget[id].alpha) < .5f)
        return;

    switch (widget[id].type)
    {
        case GUI_SPACE:  break;
        case GUI_FILLER: break;

        /* Should not include same methods, but THIS! */
        case GUI_HARRAY:
        case GUI_VARRAY:
        case GUI_HSTACK:
        case GUI_VSTACK: gui_paint_array(id); break;
        case GUI_ROOT:   gui_paint_array(id); break;
        case GUI_IMAGE:  gui_paint_image(id); break;
        case GUI_COUNT:  gui_paint_count(id); break;
        case GUI_CLOCK:  gui_paint_clock(id); break;
        default:         gui_paint_label(id); break;
    }
}

void gui_animate(int id)
{
    FUNC_VOID_CHECK_LIMITS(id);

    glTranslatef((GLfloat) (video.device_w / 2),
                 (GLfloat) (video.device_h / 2), 0.0f);

    float animation_rotation_treshold = (widget[id].alpha - 1.0f) * 4;

    /* Animation positions */
    switch (widget[id].animation_direction)
    {
        /* Single direction (No Pow) */
        case GUI_ANIMATION_N_LINEAR:
            glTranslatef(0.0f,
                         (video.device_h / -3.0f) * (widget[id].alpha - 1.0f),
                         0.0f);
            break;
        case GUI_ANIMATION_E_LINEAR:
            glTranslatef((video.device_h / -3.0f) * (widget[id].alpha - 1.0f),
                         0.0f,
                         0.0f);
            break;
        case GUI_ANIMATION_S_LINEAR:
            glTranslatef(0.0f,
                         (video.device_h / +3.0f) * (widget[id].alpha - 1.0f),
                         0.0f);
            break;
        case GUI_ANIMATION_W_LINEAR:
            glTranslatef((video.device_h / +3.0f) * (widget[id].alpha - 1.0f),
                         0.0f,
                         0.0f);
            break;

        /* Multiple directions (No Pow) */
        case GUI_ANIMATION_N_LINEAR | GUI_ANIMATION_E_LINEAR:
            glTranslatef((video.device_h / -3.0f) * (widget[id].alpha - 1.0f),
                         (video.device_h / -3.0f) * (widget[id].alpha - 1.0f),
                         0.0f);
            break;
        case GUI_ANIMATION_N_LINEAR | GUI_ANIMATION_W_LINEAR:
            glTranslatef((video.device_h / +3.0f) * (widget[id].alpha - 1.0f),
                         (video.device_h / -3.0f) * (widget[id].alpha - 1.0f),
                         0.0f);
            break;
        case GUI_ANIMATION_S_LINEAR | GUI_ANIMATION_E_LINEAR:
            glTranslatef((video.device_h / -3.0f) * (widget[id].alpha - 1.0f),
                         (video.device_h / +3.0f) * (widget[id].alpha - 1.0f),
                         0.0f);
            break;
        case GUI_ANIMATION_S_LINEAR | GUI_ANIMATION_W_LINEAR:
            glTranslatef((video.device_h / +3.0f) * (widget[id].alpha - 1.0f),
                         (video.device_h / +3.0f) * (widget[id].alpha - 1.0f),
                         0.0f);
            break;

        /* Single direction (Pow) */
        case GUI_ANIMATION_N_CURVE:
            glTranslatef(0.0f,
                         fpowf(widget[id].alpha - 1, 2) * (video.device_h / +3.0f),
                         0.0f);
            break;
        case GUI_ANIMATION_E_CURVE:
            glTranslatef(fpowf(widget[id].alpha - 1, 2) * (video.device_h / +3.0f),
                         0.0f,
                         0.0f);
            break;
        case GUI_ANIMATION_S_CURVE:
            glTranslatef(0.0f,
                         fpowf(widget[id].alpha - 1, 2) * (video.device_h / -3.0f),
                         0.0f);
            break;
        case GUI_ANIMATION_W_CURVE:
            glTranslatef(fpowf(widget[id].alpha - 1, 2) * (video.device_h / -3.0f),
                         0.0f,
                         0.0f);
            break;

        /* Multiple directions (Pow) */
        case GUI_ANIMATION_N_CURVE | GUI_ANIMATION_E_CURVE:
            glTranslatef(fpowf(widget[id].alpha - 1, 2) * (video.device_h / +3.0f),
                         fpowf(widget[id].alpha - 1, 2) * (video.device_h / +3.0f),
                         0.0f);
            break;
        case GUI_ANIMATION_N_CURVE | GUI_ANIMATION_W_CURVE:
            glTranslatef(fpowf(widget[id].alpha - 1, 2) * (video.device_h / -3.0f),
                         fpowf(widget[id].alpha - 1, 2) * (video.device_h / +3.0f),
                         0.0f);
            break;
        case GUI_ANIMATION_S_CURVE | GUI_ANIMATION_E_CURVE:
            glTranslatef(fpowf(widget[id].alpha - 1, 2) * (video.device_h / +3.0f),
                         fpowf(widget[id].alpha - 1, 2) * (video.device_h / -3.0f),
                         0.0f);
            break;
        case GUI_ANIMATION_S_CURVE | GUI_ANIMATION_W_CURVE:
            glTranslatef(fpowf(widget[id].alpha - 1, 2) * (video.device_h / -3.0f),
                         fpowf(widget[id].alpha - 1, 2) * (video.device_h / -3.0f),
                         0.0f);
            break;
    }

    /* Animation rotations */
    switch (widget[id].animation_direction)
    {
        /* Single direction (No Pow) */
        case GUI_ANIMATION_W_LINEAR:
        case GUI_ANIMATION_N_LINEAR | GUI_ANIMATION_W_LINEAR:
        case GUI_ANIMATION_S_LINEAR | GUI_ANIMATION_W_LINEAR:
            glRotatef(animation_rotation_treshold, 0.0f, 0.0f, 1.0f);
            break;
        case GUI_ANIMATION_E_LINEAR:
        case GUI_ANIMATION_N_LINEAR | GUI_ANIMATION_E_LINEAR:
        case GUI_ANIMATION_S_LINEAR | GUI_ANIMATION_E_LINEAR:
            glRotatef(animation_rotation_treshold, 0.0f, 0.0f, -1.0f);
            break;

        /* Multiple directions (No Pow) */
        case GUI_ANIMATION_W_CURVE:
        case GUI_ANIMATION_N_CURVE | GUI_ANIMATION_W_CURVE:
        case GUI_ANIMATION_S_CURVE | GUI_ANIMATION_W_CURVE:
            glRotatef(fpowf(animation_rotation_treshold, 2), 0.0f, 0.0f, 1.0f);
            break;
        case GUI_ANIMATION_E_CURVE:
        case GUI_ANIMATION_N_CURVE | GUI_ANIMATION_E_CURVE:
        case GUI_ANIMATION_S_CURVE | GUI_ANIMATION_E_CURVE:
            glRotatef(fpowf(animation_rotation_treshold, 2), 0.0f, 0.0f, -1.0f);
            break;
    }

    glTranslatef((GLfloat) (-video.device_w / 2),
                 (GLfloat) (-video.device_h / 2), 0.0f);
}

void gui_paint(int id)
{
    video_can_swap_window = 1;

    FUNC_VOID_CHECK_LIMITS(id);

    if (id && widget[id].type != GUI_FREE)
    {
        /*if (config_get_d(CONFIG_UI_HWACCEL))
        {
            video_set_perspective(70.0f, 0.001f, 10000);
            glTranslatef(video.device_w / -2.0f, video.device_h / -2.0f, -770.0f);
        }
        else*/ video_set_ortho();

        glDisable(GL_DEPTH_TEST);
        {
            gui_animate(id);

            draw_enable(GL_FALSE, GL_TRUE, GL_TRUE);
            glColor4ub(GUI_COLOR4UB);
            gui_paint_rect(id, 0, 0);

            draw_enable(GL_TRUE, GL_TRUE, GL_TRUE);
            glColor4ub(GUI_COLOR4UB);
            gui_paint_text(id);

            draw_disable();
            glColor4ub(gui_wht[0], gui_wht[1], gui_wht[2],
#if ENABLE_MOTIONBLUR!=0
                       ROUND(gui_wht[3] * video_motionblur_alpha_get()));
#else
                       ROUND(gui_wht[3]));
#endif
        }
        glEnable(GL_DEPTH_TEST);
    }

    /* Should be used within the splitview? */
#ifndef __EMSCRIPTEN__
    if (!video_get_grab() && !console_gui_shown() && cursor_st && cursor_id)
#else
    if (!video_get_grab() && cursor_st && cursor_id)
#endif
    {
        /*if (config_get_d(CONFIG_UI_HWACCEL))
        {
            video_set_perspective(70.0f, 0.001f, 10000);
            glTranslatef(video.device_w / -2.0f, video.device_h / -2.0f, -770.0f);
        }
        else*/ video_set_ortho();

        {
            glDisable(GL_DEPTH_TEST);

            draw_enable(GL_TRUE, GL_TRUE, GL_TRUE);

            gui_paint_image(cursor_id);

            draw_disable();
            glColor4ub(gui_wht[0], gui_wht[1], gui_wht[2],
#if ENABLE_MOTIONBLUR!=0
                       ROUND(gui_wht[3] * video_motionblur_alpha_get()));
#else
                       ROUND(gui_wht[3]));
#endif

            glEnable(GL_DEPTH_TEST);
        }
    }
}

void gui_set_alpha(int id, float alpha, int direction)
{
    FUNC_VOID_CHECK_LIMITS(id);

    if (id)
    {
        widget[id].alpha = CLAMP(0.0f, alpha, 1.0f);
        widget[id].animation_direction = direction;
        gui_set_alpha(widget[id].cdr, alpha, GUI_ANIMATION_NONE);
        gui_set_alpha(widget[id].car, alpha, GUI_ANIMATION_NONE);
    }
}

/*---------------------------------------------------------------------------*/

void gui_dump(int id, int d)
{
    int jd, i;

    if (id)
    {
        char *type = "undefined";

        switch (widget[id].type)
        {
            case GUI_HARRAY: type = "harray"; break;
            case GUI_VARRAY: type = "varray"; break;
            case GUI_HSTACK: type = "hstack"; break;
            case GUI_VSTACK: type = "vstack"; break;
            case GUI_FILLER: type = "filler"; break;
            case GUI_IMAGE:  type = "image";  break;
            case GUI_LABEL:  type = "label";  break;
            case GUI_COUNT:  type = "count";  break;
            case GUI_CLOCK:  type = "clock";  break;
            case GUI_BUTTON: type = "button"; break;
            case GUI_ROOT:   type = "root";   break;
        }

        for (i = 0; i < d; i++)
            printf("    ");

        printf("%04d %s\n", id, type);

        for (jd = widget[id].car; jd; jd = widget[jd].cdr)
            gui_dump(jd, d + 1);
    }
}

void gui_pulse(int id, float k)
{
    FUNC_VOID_CHECK_LIMITS(id);

    if (id)
    {
        if (widget[id].pulse_scale < k)
            widget[id].pulse_scale = k;
    }
}

void gui_timer(int id, float dt)
{
    FUNC_VOID_CHECK_LIMITS(id);

    int jd;

    if (id && widget[id].type != GUI_FREE)
    {
        widget[id].alpha_slide = 1.0f;

        if (!config_get_d(CONFIG_SCREEN_ANIMATIONS))
        {
            widget[id].offset_x = 0.0f;
            widget[id].offset_y = 0.0f;

            widget[id].alpha_slide = 1.0f;
            widget[id].alpha       = 1.0f;

            gui_set_alpha(id, 1.0f, GUI_ANIMATION_NONE);

            if (widget[id].slide_flags & GUI_REMOVE)
            {
                gui_remove(id);
                return;
            }
        }

        for (jd = widget[id].car; jd; jd = widget[jd].cdr)
            gui_timer(jd, dt);

        if ((widget[id].alpha_slide * widget[id].alpha) >= .5f)
            widget[id].pulse_scale = flerp(widget[id].pulse_scale, 1.0f, dt * 10.0f);

        if ((widget[id].flags & GUI_OFFSET) && widget[id].slide_time < widget[id].slide_delay + widget[id].slide_dur)
        {
            float alpha = 0.0f;

            widget[id].slide_time += dt;

            alpha = (widget[id].slide_time - widget[id].slide_delay) / widget[id].slide_dur;
            alpha = CLAMP(0.0f, alpha, 1.0f);

            const int at_end = (alpha >= 1.0f);

            if (widget[id].slide_flags & GUI_BACKWARD)
            {
                if (widget[id].offset_init_x == 0 && widget[id].offset_init_y == 0)
                    widget[id].alpha_slide = 1.0f - alpha;

                // Approaching offset position.

                if (widget[id].slide_flags & GUI_EASE_ELASTIC)
                    alpha = easeOutElastic(alpha);
                else if (widget[id].slide_flags & GUI_EASE_BACK)
                    alpha = easeOutBack(alpha);
                else if (widget[id].slide_flags & GUI_EASE_BOUNCE)
                    alpha = easeOutBounce(alpha);
                else
                    /* Linear interpolation. */;
            }
            else
            {
                if (widget[id].offset_init_x == 0 && widget[id].offset_init_y == 0)
                    widget[id].alpha_slide = alpha;

                // Approaching widget position.

                if (widget[id].slide_flags & GUI_EASE_ELASTIC)
                    alpha = easeInElastic(1.0f - alpha);
                else if (widget[id].slide_flags & GUI_EASE_BACK)
                    alpha = easeInBack(1.0f - alpha);
                else if (widget[id].slide_flags & GUI_EASE_BOUNCE)
                    alpha = easeInBounce(1.0f - alpha);
                else
                    alpha = 1.0f - alpha;
            }

            widget[id].offset_x = widget[id].offset_init_x * alpha;
            widget[id].offset_y = widget[id].offset_init_y * alpha;

            if (at_end && (widget[id].slide_flags & GUI_REMOVE))
            {
                gui_remove(id);
                return;
            }
        }
    }
}

int gui_point(int id, int x, int y)
{
    static int x_cache = 0;
    static int y_cache = 0;

    int jd;

    /* Reuse the last coordinates if (x,y) == (-1,-1) */

    if (x < 0 && y < 0)
        return gui_point(id, x_cache, y_cache);

    x_cache = x;
    y_cache = y;

    /* Move the cursor, if any. */

    if (cursor_id)
    {
        widget[cursor_id].x = x - widget[cursor_id].w / 2;
        widget[cursor_id].y = y - widget[cursor_id].h / 2;
    }

    /* Short-circuit check the current active widget. */

    jd = gui_search(active, x, y);

    /* If not still active, search the hierarchy for a new active widget. */

    if (jd == 0)
        jd = gui_search(id, x, y);

    /* Note hovered widget. */

    hovered = jd;

    /* If the active widget has changed, return the new active id. */

    if (jd == 0 || jd == active)
        return 0;
    else
    {
        audio_play("snd/focus.ogg", 1.0f);
        return active = jd;
    }
}

void gui_alpha(int id, float alpha)
{
    FUNC_VOID_CHECK_LIMITS(id);

    widget[id].alpha = CLAMP(0.0f, alpha, 1.0f);;
}

void gui_focus(int i)
{
    if (active != i)
        audio_play("snd/focus.ogg", 1.0f);

    active = i;
}

int gui_active(void)
{
    return active;
}

int gui_token(int id)
{
    FUNC_INT_CHECK_LIMITS(id);

    return id ? widget[id].token : 0;
}

int gui_value(int id)
{
    FUNC_INT_CHECK_LIMITS(id);

    return id ? widget[id].value : 0;
}

void gui_toggle(int id)
{
    FUNC_VOID_CHECK_LIMITS(id);

    widget[id].flags ^= GUI_HILITE;
}

/*---------------------------------------------------------------------------*/

static int gui_vert_offset(int id, int jd)
{
    /* Vertical offset between bottom of id and top of jd */

    return  widget[id].y - (widget[jd].y + widget[jd].h);
}

static int gui_horz_offset(int id, int jd)
{
    /* Horizontal offset between left of id and right of jd */

    return  widget[id].x - (widget[jd].x + widget[jd].w);
}

static int gui_vert_overlap(int id, int jd)
{
    /* Extent of vertical intersection of id and jd. */

    const int a0 = widget[id].y;
    const int a1 = widget[id].y + widget[id].h;
    const int aw = widget[id].h;
    const int b0 = widget[jd].y;
    const int b1 = widget[jd].y + widget[jd].h;
    /* const int bw = widget[jd].h; */

    return aw + MIN(b1 - a1, 0) - MAX(b0 - a0, 0);
}

static int gui_horz_overlap(int id, int jd)
{
    /* Extent of horizontal intersection of id and jd. */

    const int a0 = widget[id].x;
    const int a1 = widget[id].x + widget[id].w;
    const int aw = widget[id].w;
    const int b0 = widget[jd].x;
    const int b1 = widget[jd].x + widget[jd].w;
    /* const int bw = widget[jd].w; */

    return aw + MIN(b1 - a1, 0) - MAX(b0 - a0, 0);
}

/*---------------------------------------------------------------------------*/

/*
 * Widget navigation heuristics.
 *
 * People generally read left-to-right and top-to-bottom, and have
 * expectations on how navigation should behave. Thus, we hand-craft a
 * bunch of rules rather than devise a generic algorithm.
 *
 * Horizontal navigation only picks overlapping widgets. Closer is
 * better. Out of multiple closest widgets pick the topmost widget.
 *
 * Vertical navigation picks both overlapping and non-overlapping
 * widgets. Closer is better. Out of multiple closest widgets: if
 * overlapping, pick the leftmost widget; if not overlapping, pick the
 * one with the least negative overlap.
 */

/*
 * Leftmost/topmost is decided by the operator used to test
 * distance. A less-than will pick the first of a group of
 * equal-distance widgets, while a less-or-equal will pick the last
 * one.
 */

/* FIXME This isn't how you factor out reusable code. */

#define CHECK_HORIZONTAL \
    (o > 0 && (omin > 0 ? d <= dmin : 1))

#define CHECK_VERTICAL                                                  \
    (omin > 0 ?                                                         \
     d < dmin :                                                         \
     (o > 0 ? d <= dmin : (d < dmin || (d == dmin && o > omin))))

static int gui_stick_L(int id, int dd)
{
    int jd, kd, hd;
    int d, dmin, o, omin;

    /* Find the closest "hot" widget to the left of dd (and inside id) */

    if (id && gui_hot(id))
        return id;

    hd = 0;
    dmin = widget[dd].x - widget[id].x + 1;
    omin = INT_MIN;

    for (jd = widget[id].car; jd; jd = widget[jd].cdr)
    {
        kd = gui_stick_L(jd, dd);

        if (kd && kd != dd)
        {
            d = gui_horz_offset(dd, kd);
            o = gui_vert_overlap(dd, kd);

            if (d >= 0 && CHECK_HORIZONTAL)
            {
                hd = kd;
                dmin = d;
                omin = o;
            }
        }
    }

    return hd;
}

static int gui_stick_R(int id, int dd)
{
    int jd, kd, hd;
    int d, dmin, o, omin;

    /* Find the closest "hot" widget to the right of dd (and inside id) */

    if (id && gui_hot(id))
        return id;

    hd = 0;
    dmin = (widget[id].x + widget[id].w) - (widget[dd].x + widget[dd].w) + 1;
    omin = INT_MIN;

    for (jd = widget[id].car; jd; jd = widget[jd].cdr)
    {
        kd = gui_stick_R(jd, dd);

        if (kd && kd != dd)
        {
            d = gui_horz_offset(kd, dd);
            o = gui_vert_overlap(dd, kd);

            if (d >= 0 && CHECK_HORIZONTAL)
            {
                hd = kd;
                dmin = d;
                omin = o;
            }
        }
    }

    return hd;
}

static int gui_stick_D(int id, int dd)
{
    int jd, kd, hd;
    int d, dmin, o, omin;

    /* Find the closest "hot" widget below dd (and inside id) */

    if (id && gui_hot(id))
        return id;

    hd = 0;
    dmin = widget[dd].y - widget[id].y + 1;
    omin = INT_MIN;

    for (jd = widget[id].car; jd; jd = widget[jd].cdr)
    {
        kd = gui_stick_D(jd, dd);

        if (kd && kd != dd)
        {
            d = gui_vert_offset(dd, kd);
            o = gui_horz_overlap(dd, kd);

            if (d >= 0 && CHECK_VERTICAL)
            {
                hd = kd;
                dmin = d;
                omin = o;
            }
        }
    }

    return hd;
}

static int gui_stick_U(int id, int dd)
{
    int jd, kd, hd;
    int d, dmin, o, omin;

    /* Find the closest "hot" widget above dd (and inside id) */

    if (id && gui_hot(id))
        return id;

    hd = 0;
    dmin = (widget[id].y + widget[id].h) - (widget[dd].y + widget[dd].h) + 1;
    omin = INT_MIN;

    for (jd = widget[id].car; jd; jd = widget[jd].cdr)
    {
        kd = gui_stick_U(jd, dd);

        if (kd && kd != dd)
        {
            d = gui_vert_offset(kd, dd);
            o = gui_horz_overlap(dd, kd);

            if (d >= 0 && CHECK_VERTICAL)
            {
                hd = kd;
                dmin = d;
                omin = o;
            }
        }
    }

    return hd;
}

/*---------------------------------------------------------------------------*/

static int gui_wrap_L(int id, int dd)
{
    int jd, kd;

    if ((jd = gui_stick_L(id, dd)) == 0)
        for (jd = dd; (kd = gui_stick_R(id, jd)); jd = kd)
            ;

    return jd;
}

static int gui_wrap_R(int id, int dd)
{
    int jd, kd;

    if ((jd = gui_stick_R(id, dd)) == 0)
        for (jd = dd; (kd = gui_stick_L(id, jd)); jd = kd)
            ;

    return jd;
}

static int gui_wrap_U(int id, int dd)
{
    int jd, kd;

    if ((jd = gui_stick_U(id, dd)) == 0)
        for (jd = dd; (kd = gui_stick_D(id, jd)); jd = kd)
            ;

    return jd;
}

static int gui_wrap_D(int id, int dd)
{
    int jd, kd;

    if ((jd = gui_stick_D(id, dd)) == 0)
        for (jd = dd; (kd = gui_stick_U(id, jd)); jd = kd)
            ;

    return jd;
}

/*---------------------------------------------------------------------------*/

int gui_stick(int id, int a, float v, int bump)
{
    int jd = 0;

    if (!bump)
        return 0;

    /* Find a new active widget in the direction of joystick motion. */

    if      (config_tst_d(CONFIG_JOYSTICK_AXIS_X0, a))
    {
        if (v + axis_offset[0] < 0) jd = gui_wrap_L(id, active);
        if (v + axis_offset[0] > 0) jd = gui_wrap_R(id, active);
    }
    else if (config_tst_d(CONFIG_JOYSTICK_AXIS_Y0, a))
    {
        if (v + axis_offset[1] < 0) jd = gui_wrap_U(id, active);
        if (v + axis_offset[1] > 0) jd = gui_wrap_D(id, active);
    }

    /* If the active widget has changed, return the new active id. */

    if (jd == 0 || jd == active)
        return 0;
    else
    {
        audio_play("snd/focus.ogg", 1.0f);
        return active = jd;
    }
}

int gui_click(int b, int d)
{
    if (b == SDL_BUTTON_LEFT)
    {
        if (d)
        {
            clicked = hovered;
            return 0;
        }
        else
        {
            int c = (clicked && clicked == hovered);
            clicked = 0;
            return c;
        }
    }
    return 0;
}

/*---------------------------------------------------------------------------*/

int gui_navig(int id, int total, int first, int step)
{
    return gui_navig_full(id, total, first, step, 0);
}

int gui_navig_full(int id, int total, int first, int step, int back_disabled)
{
    int pages = (int) ceil((double) total / step);
    int page = first / step + 1;

    int prev = (page > 1);
    int next = (page < pages);

    int jd, kd;

    if ((jd = gui_hstack(id)))
    {
        if (next || prev)
        {
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
            if (current_platform == PLATFORM_PC && !console_gui_shown())
#endif
            {
#ifdef SWITCHBALL_GUI
                gui_maybe_img(jd, "gui/navig/arrow_right_disabled.png", "gui/navig/arrow_right.png", GUI_NEXT, GUI_NONE, next);
#else
                const int icn_id = gui_maybe(jd, GUI_TRIANGLE_RIGHT, GUI_NEXT, GUI_NONE, next);
                gui_set_font(icn_id, "ttf/DejaVuSans-Bold.ttf");
#endif
            }

            if ((kd = gui_label(jd, "99999/99999", GUI_XS, gui_wht, gui_wht)))
            {
                char str[12];
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
                sprintf_s(str, 12,
#else
                sprintf(str,
#endif
                        "%d/%d", page, pages);
                gui_set_label(kd, str);
            }
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
            if (current_platform == PLATFORM_PC && !console_gui_shown())
#endif
            {
#ifdef SWITCHBALL_GUI
                gui_maybe_img(jd, "gui/navig/arrow_left_disabled.png", "gui/navig/arrow_left.png", GUI_PREV, GUI_NONE, prev);
#else
                const int icn_id = gui_maybe(jd, GUI_TRIANGLE_LEFT, GUI_PREV, GUI_NONE, prev);
                gui_set_font(icn_id, "ttf/DejaVuSans-Bold.ttf");
#endif
            }
        }
#if NB_HAVE_PB_BOTH==1 && !defined(__EMSCRIPTEN__)
        if (current_platform == PLATFORM_PC && !console_gui_shown())
#endif
        {
            gui_space(jd);

            if ((kd = gui_hstack(jd)))
            {
                const int icn_id = gui_label(kd, GUI_CROSS, GUI_SML, back_disabled ? gui_gry : gui_red, back_disabled ? gui_gry : gui_red);
                gui_label(kd, _("Back"), GUI_SML, back_disabled ? gui_gry : gui_wht, back_disabled ? gui_gry : gui_wht);
                gui_set_font(icn_id, "ttf/DejaVuSans-Bold.ttf");

                gui_set_state(kd, back_disabled ? GUI_NONE : GUI_BACK, 0);
                gui_set_rect(kd, GUI_ALL);
            }
        }
    }
    return jd;
}

#ifdef SWITCHBALL_GUI
int gui_maybe_img(int id, const char *dimage, const char *eimage, int etoken, int dtoken, int enabled)
{
    int bd = gui_image(id, enabled ? eimage : dimage, widget[digit_id[1][0]].w * 0.8f, widget[digit_id[1][0]].h * 0.4f);

    gui_set_state(bd, enabled ? etoken : dtoken, 0);

    return bd;
}
#else
int gui_maybe(int id, const char *label, int etoken, int dtoken, int enabled)
{
    int bd;

    if (!enabled)
    {
        bd = gui_state(id, label, GUI_SML, dtoken, 0);
        gui_set_color(bd, gui_gry, gui_gry);
    }
    else
        bd = gui_state(id, label, GUI_SML, etoken, 0);

    return bd;
}
#endif

/*---------------------------------------------------------------------------*/
