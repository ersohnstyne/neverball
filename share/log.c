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

#include <stdio.h>
#include <stdlib.h>

#include "log.h"
#include "common.h"
#ifndef VERSION
#include "version.h"
#endif
#include "fs.h"

#if _DEBUG && _MSC_VER
#ifndef _CRTDBG_MAP_ALLOC
#pragma message(__FILE__": Missing CRT-Debugger include header, recreate: crtdbg.h")
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#endif

static char    log_header[MAXSTR];
static fs_file log_fp;

#if NB_EOS_SDK==1
#define TEMP_LOG_INFO  "[EOS INFO] %s"
#define TEMP_LOG_ERROR "[EOS ERROR] %s"
#else
#define TEMP_LOG_INFO  "[i] %s"
#define TEMP_LOG_ERROR "[!] %s"
#endif

/*---------------------------------------------------------------------------*/

void log_printf(const char *fmt, ...)
{
    char *str;
    int len;

    va_list ap;

    va_start(ap, fmt);
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    //len = 1 + vsnprintf_s(NULL, 0, MAXSTR, fmt, ap);
    len = 1 + vsnprintf(NULL, 0, fmt, ap);
#else
    len = 1 + vsnprintf(NULL, 0, fmt, ap);
#endif
    va_end(ap);

    if ((str = (char *) malloc(len)))
    {
        va_start(ap, fmt);
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        //vsnprintf_s(str, len, MAXSTR, fmt, ap);
        vsnprintf(str, len, fmt, ap);
#else
        vsnprintf(str, len, fmt, ap);
#endif
        va_end(ap);

        fputs(str, stdout);
        fflush(stdout);

        if (log_fp)
        {
            /* These are printfs to get us CRLF conversion. */

            if (log_header[0])
            {
                fs_printf(log_fp, "%s\n", log_header);
                log_header[0] = 0;
            }

            fs_printf(log_fp, TEMP_LOG_INFO, str);
            fs_flush(log_fp);
        }
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        else
        {
            fprintf_s(stdout, TEMP_LOG_INFO, str);
            printf_s(TEMP_LOG_INFO, str);
        }
#else
        else
        {
            fprintf(stdout, TEMP_LOG_INFO, str);
            printf(TEMP_LOG_INFO, str);
        }
#endif

#if defined(_DEBUG)
        OutputDebugStringA("[i] NB INFO: ");
        OutputDebugStringA(str);
#endif

        free(str);
    }
}

void log_errorf(const char *fmt, ...)
{
    char *str;
    int len;

    va_list ap;

    va_start(ap, fmt);
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
    //len = 1 + vsnprintf_s(NULL, 0, MAXSTR, fmt, ap);
    len = 1 + vsnprintf(NULL, 0, fmt, ap);
#else
    len = 1 + vsnprintf(NULL, 0, fmt, ap);
#endif
    va_end(ap);

    if ((str = (char *) malloc(len)))
    {
        va_start(ap, fmt);
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        //vsnprintf_s(str, len, MAXSTR, fmt, ap);
        vsnprintf(str, len, fmt, ap);
#else
        vsnprintf(str, len, fmt, ap);
#endif
        va_end(ap);

        fputs(str, stderr);
        fflush(stderr);

        if (log_fp)
        {
            /* These are printfs to get us CRLF conversion. */

            if (log_header[0])
            {
                fs_printf(log_fp, "%s\n", log_header);
                log_header[0] = 0;
            }

            fs_printf(log_fp, TEMP_LOG_ERROR, str);
            fs_flush(log_fp);
        }
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
        else
        {
            fprintf_s(stderr, TEMP_LOG_ERROR, str);
            printf_s(TEMP_LOG_ERROR, str);
        }
#else
        else
        {
            fprintf(stderr, TEMP_LOG_ERROR, str);
            printf(TEMP_LOG_ERROR, str);
        }
#endif

#if defined(_DEBUG)
        OutputDebugStringA("[!] NB ERROR: ");
        OutputDebugStringA(str);
#endif

        free(str);
    }
}

/*---------------------------------------------------------------------------*/

void log_init(const char *name, const char *path)
{
    if (!log_fp)
    {
#ifdef FS_VERSION_1
        log_fp = fs_open(path, "w+");
#else
        log_fp = fs_open_write(path);
#endif

        if (log_fp)
        {
            /* Printed on first message. */

#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            sprintf_s(log_header,
#else
            sprintf(log_header,
#endif
                    "%s - %s %s",
                    date_to_str(time(NULL)),
                    name, VERSION);
        }
        else
        {
#if defined(_DEBUG)
            OutputDebugStringA("[!] NB ERROR: ");
            OutputDebugStringA("Failure to open ");
            OutputDebugStringA(path);
            OutputDebugStringA("!: ");
            OutputDebugStringA(fs_error());
            OutputDebugStringA("\n");
#endif
#if _WIN32 && !defined(__EMSCRIPTEN__) && !_CRT_SECURE_NO_WARNINGS
            fprintf_s(stderr, "Failure to open %s!: %s\n", path, fs_error());
            printf_s("Failure to open %s!: %s\n", path, fs_error());
#else
            fprintf(stderr, "Failure to open %s!: %s\n", path, fs_error());
            printf("Failure to open %s!: %s\n", path, fs_error());
#endif
        }
    }
}

void log_quit(void)
{
    if (log_fp)
    {
        fs_close(log_fp);
        log_header[0] = 0;
    }
}

/*---------------------------------------------------------------------------*/
