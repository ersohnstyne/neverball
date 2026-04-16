/*
 * Copyright (C) 2026 Microsoft / Neverball authors / Jānis Rūcis
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

/*
 * Used with c++ signal protection from events.
 */

#if defined(_DEBUG) && _WIN32
#include <Windows.h>
#include <DbgHelp.h>
#pragma comment(lib, "dbghelp.lib")
#elif defined(_DEBUG)
#include <execinfo.h>
#endif

#include "log.h"
#include "common.h"

#include "dbg_config.h"

/*---------------------------------------------------------------------------*/

static int  dbg_signum;
static char dbg_strerror[256];

#if defined(_DEBUG) && _WIN32
static void  *dbg_frames[20];
static char **dbg_strings;
#endif

#if __cplusplus
extern "C" {
#endif

int         GameDbg_GetSigNum(void) { return dbg_signum; }
void        GameDbg_ClrSigNum(void) { dbg_signum = 0; }
const char *GameDbg_GetError(void)  { return dbg_strerror; }

void GameDbg_SigHandler(int signum)
{
    dbg_signum = signum;

    GameDbg_Check_SegPerformed();

#ifdef _DEBUG
    log_errorf("DEBUG CODE RUNTIME ERROR!\n");
#if _WIN32
    void *dbg_frames[20];
    int nptrs = CaptureStackBackTrace(1, 20, dbg_frames, 0);

    HANDLE _handle = GetCurrentProcess();

    if (SymInitialize(_handle, 0, 1))
    {
        SYMBOL_INFO *_symbol  = (SYMBOL_INFO *) calloc(sizeof (SYMBOL_INFO) + MAXSTR, 1);
        _symbol->MaxNameLen   = MAXSTR;
        _symbol->SizeOfStruct = sizeof (SYMBOL_INFO);

        for (int i = 0; i < nptrs; i++)
        {
            char dbg_final_text[MAXSTR];

            DWORD64 _address = (DWORD64) (dbg_frames[i]);
            DWORD64 _displacement = 0;
            IMAGEHLP_LINE64 _line;

            if (SymFromAddr(_handle, _address, 0, _symbol))
            {
                if (SymGetLineFromAddr64(_handle, _address, &_displacement, &_line))
                    sprintf_s(dbg_final_text, MAXSTR, "    %s(%u): %s (0x%16X)\n", _line.FileName, _line.LineNumber, _symbol->Name, _symbol->Address);
                else sprintf_s(dbg_final_text, MAXSTR, "    %s (0x%16X)\n", _symbol->Name, _symbol->Address);

                log_errorf(dbg_final_text);
            }
            else log_errorf("    [Unknown symbol] (0x%16X)\n", _symbol->Address);
        }

        free(_symbol);
        SymCleanup(_handle);

        __debugbreak();
    }
#else
    void *buffer[240];
    int   nptrs = backtrace(buffer, 240);
    dbg_strings = backtrace_symbols(buffer, nptrs);

    for (int i = 0; i < nptrs; i++)
        log_errorf("    %s\n", strings[i]);

    free(dbg_strings);
#endif
#endif

    exit((signum == 2 || signum == 21) ? 0 : 1);
}

void GameDbg_Check_SegPerformed(void)
{
    memset(dbg_strerror, 0, 256);
#if _WIN32 && _MSC_VER
    switch (dbg_signum) {
        case 0: return;
        case SIGHUP:
            DW_FORMAT_MSG(ERROR_TIMEOUT, dbg_strerror, 255); break;
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
    MessageBoxA(0, dbg_strerror, "Debug error!", MB_ICONERROR);
#else
    DW_FORMAT_MSG(dbg_strerror);
    log_errorf("Debug error: %s\n", dbg_strerror);
#endif
    GameDbg_ClrSigNum();

#if __cplusplus
    throw dbg_strerror;
#endif
}

#if __cplusplus
}
#endif
