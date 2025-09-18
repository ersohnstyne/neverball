/*
 * Copyright (C) 2025 Microsoft / Neverball authors / Jānis Rūcis
 * Copyright (C) 2003 Robert Kooima
 * Copyright (C) 2025 Jānis Rūcis
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

/*---------------------------------------------------------------------------*/

#ifndef _CONSOLE
#define _CONSOLE 1
#endif

#include <stdio.h>
#include <setjmp.h>
#include <stdlib.h>

#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)

#include "fs.h"
#include "mapclib.h"

/*
 * TODO: If you want to compile some balls and/or geometrys only,
 * use the option as --skip_verify.
 * - Ersohn Styne
 */

#define MAPC_FILEEXT_CSOL ".csol"
#define MAPC_FILEEXT_SOL  ".sol"

#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char* argv[])
{
    mapc_context ctx = NULL;

    if (!fs_init(argc > 0 ? argv[0] : NULL))
    {
        fprintf(stderr, "Failure to initialize virtual file system!: %s\n", fs_error());
        return 1;
    }

    if (!mapc_init(&ctx))
    {
        fprintf(stderr, "Failure to initialize mapc context!: %s\n", fs_error());
        return 1;
    }

    if (!mapc_opts(ctx, argc, argv))
    {
        mapc_quit(&ctx);
        return 1;
    }

    if (!mapc_compile(ctx))
    {
        mapc_quit(&ctx);
        return 1;
    }

    mapc_dump(ctx);
    mapc_quit(&ctx);

    return 0;
}

#else

#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char *argv[])
{
    printf("Nintendo consoles not supported!\n");

    return 0;
}

#endif
