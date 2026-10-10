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

#if !defined(__NDS__) && !defined(__3DS__) && \
    !defined(__GAMECUBE__) && !defined(__WII__) && !defined(__WIIU__) && \
    !defined(__SWITCH__)
#if NB_HAVE_PB_BOTH==1 && NB_PB_SDL3==1
# define SDL_MAIN_HANDLED
# include <SDL3/SDL.h>
# if _MSC_VER >= 1950 && !defined(SDL_MAIN_HANDLED)
#  pragma comment(lib, "SDL3.lib")
#  pragma comment(lib, "SDL3_uclibc.lib")
#  pragma comment(lib, "SDL3_ttf.lib")
# elif _MSC_VER >= 1950 && defined(SDL_MAIN_HANDLED)
#  include <Windows.h>
# else
#  error SDL3 for Windows requires latest Visual Studio C++ Version 14.50!
# endif
#elif _WIN32 && __MINGW32__
# include <SDL2/SDL.h>
#elif _WIN32 && _MSC_VER
/* HACK: Redirected to official function WinMain() - Ersohn Styne */
# define SDL_MAIN_HANDLED
# include <SDL.h>
# pragma comment(lib, "SDL2.lib")
# pragma comment(lib, "SDL2_ttf.lib")
# if _MSC_VER && !defined(SDL_MAIN_HANDLED)
#  pragma comment(lib, "SDL2main.lib")
# elif _MSC_VER && defined(SDL_MAIN_HANDLED)
#  include <Windows.h>
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

#include <TlHelp32.h>

#include "main_share.h"

/*---------------------------------------------------------------------------*/

#define WIN32_OUTPUTDEBUGSTRING_FILE_PREFIX_INFO \
    "[i] NB INFO: " __FILE__ ": "

#define WIN32_OUTPUTDEBUGSTRING_FILE_PREFIX_ERROR \
    "[!] NB ERROR: " __FILE__ ": "

/*---------------------------------------------------------------------------*/

static float time_to_next_update;

const char suspicious_processes[4][256] = {
    "cheatengine.exe",
    "ollydbg.exe",
    "x64dbg.exe",
    "processhacker.exe"
};

static void Win32_ToLowerCase_ANSI(char *str)
{
    for (size_t i = 0; str[i] != '\0'; i++)
        str[i] = (char) tolower(str[i]);
}

#if _WIN32
/* Structure definition for OSVERSIONINFOEXW */
typedef LONG (WINAPI *RtlGetVersionPtr) (PRTL_OSVERSIONINFOW);

static int DetectSuspiciousProcess()
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof (pe);

    if (Process32First(snapshot, &pe))
    {
        do {
            char executable_file_name[256];
            strcpy_s(executable_file_name, 256, pe.szExeFile);
            Win32_ToLowerCase_ANSI(executable_file_name);

            for (int i = 0; i < 4; i++)
            {
                if (strcmp(executable_file_name, suspicious_processes[i]) == 0)
                {
                    CloseHandle(snapshot);
                    return 1;
                }
            }
        } while (Process32Next(snapshot, &pe));
    }

    CloseHandle(snapshot);
    return 0;
}

static int GetWindowsVersion(unsigned long *major, unsigned long *minor, unsigned long *build)
{
    if (!major || !minor || !build) return 0;

    HMODULE hMod = GetModuleHandleA("ntdll.dll");
    if (!hMod) return 0;
    
    RtlGetVersionPtr fxPtr = (RtlGetVersionPtr) GetProcAddress(hMod, "RtlGetVersion");
    if (!fxPtr) return 0;

    RTL_OSVERSIONINFOW rovi = {0};
    rovi.dwOSVersionInfoSize = sizeof (rovi);

    if (fxPtr(&rovi) != 0) return 0;

    if (major) *major = rovi.dwMajorVersion;
    if (minor) *minor = rovi.dwMinorVersion;
    if (build) *build = rovi.dwBuildNumber;
    return 1;
}

static int Win32_EnsureMainInit(void)
{
    /**
     * HACK: The minimum Windows 11 OS is 10.0.22621.X - Ersohn Styne
     *
     * Windows 11 minimum requirement version number:
     * * M = 10
     * * m = 0
     * * b = 22621
     *
     * Windows 11 minimum requirement version number for recommendations:
     * * M = 10
     * * m = 0
     * * b = 26100
     *
     * M = Major - m = Minor - b = Build
     */
    DWORD major = 0, minor = 0, build = 0;

    if (!GetWindowsVersion(&major, &minor, &build))
    {
        OutputDebugStringA(WIN32_OUTPUTDEBUGSTRING_FILE_PREFIX_ERROR "Can't get Windows OS version!\n");
        return 0;
    }

    if (major < 10)
    {
        MessageBoxA(0, "This game requires Windows 10 or later version!", "Error", MB_ICONERROR);
        return 0;
    }

    if (build < 26100)
    {
        MessageBoxA(0, "This game requires Windows 10.0.26100 or later version!", "Error", MB_ICONERROR);
        return 0;
    }

    OutputDebugStringA(WIN32_OUTPUTDEBUGSTRING_FILE_PREFIX_INFO "Found Windows 11 RTL!\n");

#if NDEBUG
    if (IsDebuggerPresent() ||
        CheckRemoteDebuggerPresent(GetCurrentProcess(), NULL))
    {
        MessageBoxA(0, "Debugging is not available with release builds.", "Error", MB_ICONERROR);
        return 0;
    }

    if (DetectSuspiciousProcess())
    {
        MessageBoxA(0, "Suspicious process is not linkable with release builds.", "Error", MB_ICONERROR);
        return 0;
    }
#endif

    return 1;
}

#ifdef IN_TESTING_ANTICHEAT
int Win32_EnsureUpdate(float dt)
{
    time_to_next_update -= dt;

    while (time_to_next_update < 0)
    {
        time_to_next_update += 10.0f;

#if NDEBUG
        if (IsDebuggerPresent() ||
            CheckRemoteDebuggerPresent(GetCurrentProcess(), NULL) ||
            DetectSuspiciousProcess())
        {
            /* BOOM! */

            Sleep(3000);
            exit(1);

            return 0;
        }
#endif
    }

    /* All good! */

    return 1;
}
#endif
#endif

/*---------------------------------------------------------------------------*/

#if _WIN32 && _MSC_VER && defined(SDL_MAIN_HANDLED)
/*
 * HACK: Tell, which windows command line options has available,
 * when using WinMain() with SDL_MAIN_HANDLED. - Ersohn Styne
 */
int
#if !defined(_MAC)
#if defined(_M_CEE_PURE)
__clrcall
#else
WINAPI
#endif
#else
CALLBACK
#endif
WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int sw)
{
    if (!Win32_EnsureMainInit())
        return 1;

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

    time_to_next_update = 10.0f;

    SDL_SetMainReady();
    const int out_return = main_share(argc, argv);

    OutputDebugStringA(WIN32_OUTPUTDEBUGSTRING_FILE_PREFIX_INFO "Quitting Win32 runtime...\n");

    for (UINT i = 0; i < argc; ++i)
        HeapFree(win32_process_heap, 0, argv[i]);

    HeapFree(win32_process_heap, 0, argv);

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

#undef WIN32_OUTPUTDEBUGSTRING_FILE_PREFIX_INFO
#undef WIN32_OUTPUTDEBUGSTRING_FILE_PREFIX_ERROR
