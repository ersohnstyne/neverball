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

#ifndef DBG_CONFIG_H
#define DBG_CONFIG_H

#if _MSC_VER
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#if _DEBUG
#include <crtdbg.h>
#ifndef _CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC
#endif
#endif
#endif

#include <signal.h>

#ifndef SIGINT
#define SIGINT   2 // interrupt
#endif
#ifndef SIGILL
#define SIGILL   4 // illegal instruction - invalid function image
#endif
#ifndef SIGTRAP
#define SIGTRAP  5 // Trace/breakpoint trap
#endif
#ifndef SIGBUS
#define SIGBUS   7 // Bus error (bad memory access)
#endif
#ifndef SIGFPE
#define SIGFPE   8 // Floating-point exception
#endif
#ifndef SIGSEGV
#define SIGSEGV  11 // invalid memory reference - segment violation
#endif
#ifndef SIGTERM
#define SIGTERM  15 // Software termination signal from kill
#endif
#ifndef SIGBREAK
#define SIGBREAK 21 // Ctrl-Break sequence
#endif
#ifndef SIGABRT
#define SIGABRT  22 // abnormal termination triggered by abort call
#endif

/* Debug tools */

// The standard Windows Game DBG
#define GAMEDBG_GETSTRERROR \
    (GameDbg_GetError() ? \
        GameDbg_GetError() : \
        "Unknown error")

// TODO: To use this, requires SDL2.h and SDL2.lib
#define GAMEDBG_GETSTRERROR_CHOICES_SDL \
    SDL_GetError() ? \
        SDL_GetError() : \
        GAMEDBG_GETSTRERROR

#define GAMEDBG_CHECK_SEGMENTATIONS(func) \
    do { func; if (GameDbg_GetSigInt() != 0) { \
        GameDbg_Check_SegPerformed(); \
        GameDbg_ClrSigInt(); \
    } } while(0)

#define GAMEDBG_CHECK_SEGMENTATIONS_BOOL(func) \
    do { func; if (GameDbg_GetSigInt() != 0) { \
        GameDbg_Check_SegPerformed(); \
        if (GameDbg_GetSigInt() != 0) { GameDbg_ClrSigInt(); return 0; } \
        GameDbg_ClrSigInt(); \
    } } while(0)

#if _WIN32
#define DW_FORMAT_MSG(dwEC, outStr, len) \
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, 0, dwEC, \
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), outStr, len, 0)
#else
#ifdef GetLastError
#define GetLastError() strerror(errno)
#endif
#define DW_FORMAT_MSG(dwEC, outStr, len) \
    outStr = strerror(errno)
#endif

int         GameDbg_GetSigInt(void);
void        GameDbg_ClrSigInt(void);
const char *GameDbg_GetError(void);

void GameDbg_SigNum_CtrlC(int);
void GameDbg_SigNum_ElemAddr(int);
void GameDbg_SigNum_Breakpt(int);
void GameDbg_SigNum_FloatPoint(int);
void GameDbg_SigNum_Segments(int);
void GameDbg_SigNum_Term(int);
void GameDbg_SigNum_CtrlBreak(int);
void GameDbg_SigNum_Abort(int);

void GameDbg_Check_SegPerformed(void);

/*#define GAMEDBG_SIGFUNC_PREPARE \
    signal(SIGINT, GameDbg_SigNum_CtrlC); \
    signal(SIGILL, GameDbg_SigNum_ElemAddr); \
    signal(SIGTRAP, GameDbg_SigNum_Breakpt); \
    signal(SIGFPE, GameDbg_SigNum_FloatPoint); \
    signal(SIGSEGV, GameDbg_SigNum_Segments); \
    signal(SIGTERM, GameDbg_SigNum_Term); \
    signal(SIGBREAK, GameDbg_SigNum_CtrlBreak); \
    signal(SIGABRT, GameDbg_SigNum_Abort);*/

#define GAMEDBG_SIGFUNC_PREPARE \
    signal(SIGSEGV, GameDbg_SigNum_Segments);

#endif