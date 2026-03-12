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

#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)

#if _WIN32 && __MINGW32__
# include <SDL2/SDL.h>
#elif _WIN32 && _MSC_VER
# define SDL_MAIN_HANDLED
# include <SDL.h>
# pragma comment(lib, "SDL2.lib")
# if _MSC_VER && !defined(SDL_MAIN_HANDLED)
#  pragma comment(lib, "SDL2main.lib")
# elif _MSC_VER && defined(SDL_MAIN_HANDLED)
#  include <Windows.h>
#  include <ShlObj.h>
# endif
# ifndef _CRTDBG_MAP_ALLOC
#  pragma message(__FILE__": Missing _CRT_MAP_ALLOC, recreate: _CRTDBG_MAP_ALLOC + crtdbg.h")
#  define _CRTDBG_MAP_ALLOC
#  include <crtdbg.h>
# endif
#elif _WIN32
# error Security compilation error: No target include file in path for Windows specified!
#else
# include <SDL.h>
#endif

#endif

#include "main_share.h"

#if _WIN32 && _MSC_VER && defined(SDL_MAIN_HANDLED)
/*
 * HACK: Tell, which windows command line options has available,
 * when using WinMain() with SDL_MAIN_HANDLED. - Ersohn Styne
 */
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int sw)
{
#define WIN32_OUTPUTDEBUGSTRING_FILE_PREFIX_INFO \
    "[i] NB INFO: " __FILE__ ": "

#define WIN32_OUTPUTDEBUGSTRING_FILE_PREFIX_ERROR \
    "[!] NB ERROR: " __FILE__ ": "

    OutputDebugStringA(WIN32_OUTPUTDEBUGSTRING_FILE_PREFIX_INFO "Starting Win32 runtime...\n");
    HANDLE win32_process_heap = GetProcessHeap();

    UINT argc = 0;
    const LPWSTR *argvw = CommandLineToArgvW(GetCommandLineW(), &argc);

    char **argv = (char **) HeapAlloc(win32_process_heap, HEAP_ZERO_MEMORY, (argc + 1) * sizeof (*argv));
    if (!argv) {
        OutputDebugStringA(WIN32_OUTPUTDEBUGSTRING_FILE_PREFIX_ERROR "Can't allocate memory for argv!\n");
        return 1;
    }

    for (UINT i = 0; i < argc; i++) {
        const UINT len = wcslen(argvw[i]);
        size_t num_converted_chars = 0;

        argv[i] = (char *) HeapAlloc(win32_process_heap, HEAP_ZERO_MEMORY, (size_t) len + 1);
        if (!argv[i]) {
            OutputDebugStringA(WIN32_OUTPUTDEBUGSTRING_FILE_PREFIX_ERROR "Can't allocate memory for argv with current index!\n");
            return 1;
        }

        wcstombs_s(&num_converted_chars, argv[i], (size_t) len + 1, argvw[i], (size_t) len + 1);
    }

    SDL_SetMainReady();
    const int out_return = main_share(argc, argv);

    OutputDebugStringA(WIN32_OUTPUTDEBUGSTRING_FILE_PREFIX_INFO "Quitting Win32 runtime...\n");

    for (UINT i = 0; i < argc; ++i)
        HeapFree(win32_process_heap, 0, argv[i]);

    HeapFree(win32_process_heap, 0, argv);

#undef WIN32_OUTPUTDEBUGSTRING_FILE_PREFIX_INFO
#undef WIN32_OUTPUTDEBUGSTRING_FILE_PREFIX_ERROR

#if _WIN32 && _MSC_VER && _DEBUG && defined(_CRTDBG_MAP_ALLOC)
    _CrtDumpMemoryLeaks();
#endif

    return out_return;
}
#else
int main(int argc, char *argv[])
{
    return main_share(argc, argv);
}
#endif
