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

/*
 * Used with c++ signal protection from events.
 */

#include "dbg_config.h"

/*---------------------------------------------------------------------------*/

static int dbg_sigint = 0;
static char dbg_strerror[256];

int GameDbg_GetSigInt(void) { return dbg_sigint; }
void GameDbg_ClrSigInt(void) { dbg_sigint = 0; }
const char* GameDbg_GetError(void) { return dbg_strerror; }

void GameDbg_SigNum_CtrlC(int signum) { dbg_sigint = signum; exit(0); }
void GameDbg_SigNum_ElemAddr(int signum) { dbg_sigint = signum; }
void GameDbg_SigNum_Breakpt(int signum) { dbg_sigint = signum; }
void GameDbg_SigNum_FloatPoint(int signum) { dbg_sigint = signum; }
void GameDbg_SigNum_Segments(int signum) { dbg_sigint = signum; }
void GameDbg_SigNum_Term(int signum) { dbg_sigint = signum; exit(0); }
void GameDbg_SigNum_CtrlBreak(int signum) { dbg_sigint = signum; exit(0); }
void GameDbg_SigNum_Abort(int signum) { dbg_sigint = signum; exit(3); }

void GameDbg_Check_SegPerformed(void)
{ 
    memset(dbg_strerror, 0, 256);
    switch (dbg_sigint) {
    case 0: return;
    case SIGINT:
        DW_FORMAT_MSG(ERROR_DBG_CONTROL_C, dbg_strerror, 255); break;
    case SIGILL:
        DW_FORMAT_MSG(ERROR_ILLEGAL_ELEMENT_ADDRESS, dbg_strerror, 255); break;
    case SIGTRAP:
        DW_FORMAT_MSG(ERROR_SEGMENT_NOTIFICATION, dbg_strerror, 255); break;
    case SIGFPE:
        DW_FORMAT_MSG(ERROR_ILLEGAL_FLOAT_CONTEXT, dbg_strerror, 255); break;
    case SIGTERM:
        DW_FORMAT_MSG(ERROR_DBG_TERMINATE_PROCESS, dbg_strerror, 255); break;
    case SIGBREAK:
        DW_FORMAT_MSG(ERROR_DBG_CONTROL_BREAK, dbg_strerror, 255); break;
    default:
        DW_FORMAT_MSG(GetLastError(), dbg_strerror, 255);
    }
    log_errorf("Debug error: %s\n", dbg_strerror);
#if _WIN32
    MessageBoxA(0, dbg_strerror, "Debug error!", MB_ICONERROR);
#endif
    dbg_sigint = 0;
#if __cplusplus
    throw dbg_strerror;
#endif
}