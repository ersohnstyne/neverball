<<<<<<< HEAD
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

=======
>>>>>>> 28382048d48bba0d0f12edf53fce7cdb3cb3bc86
#ifndef MAPCLIB_H
#define MAPCLIB_H

#include <setjmp.h>

<<<<<<< HEAD
typedef struct mapc_context* mapc_context;

int mapc_init(mapc_context* ctx_ptr);
void mapc_quit(mapc_context* ctx_ptr);

int mapc_opts(mapc_context ctx, int argc, char* argv[]);

void mapc_set_src(mapc_context ctx, const char* src);
void mapc_set_dst(mapc_context ctx, const char* dst);
=======
typedef struct mapc_context *mapc_context;

int mapc_init(mapc_context *ctx_ptr);
void mapc_quit(mapc_context *ctx_ptr);

int mapc_opts(mapc_context ctx, int argc, char *argv[]);

void mapc_set_src(mapc_context ctx, const char *src);
void mapc_set_dst(mapc_context ctx, const char *dst);
>>>>>>> 28382048d48bba0d0f12edf53fce7cdb3cb3bc86

int mapc_compile(mapc_context ctx);

void mapc_dump(mapc_context ctx);

<<<<<<< HEAD
#endif
=======
#endif
>>>>>>> 28382048d48bba0d0f12edf53fce7cdb3cb3bc86
