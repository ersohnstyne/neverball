/*
 * Copyright (C) 2026 Microsoft / Neverball authors / Ersohn Styne
 *
 * PENNYBALL is  free software; you can redistribute  it and/or modify
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

#if NB_HAVE_PB_BOTH==1 && _WIN32 && _MSC_VER
#include <Windows.h> /* FindClose(), FindFirstFile(), FindNextFile() */
#endif

#include <stdio.h>
#include <sys/stat.h>

#include <vector>
#include <string>

extern "C" {
#include "base_config.h"

#include "fs.h"
#include "common.h"
#include "list.h"

#include "mapmarkers.h"

#include "solid_draw.h"

#include "config.h"
#include "glext.h"
#include "log.h"
}

#if _DEBUG && _MSC_VER
#ifndef _CRTDBG_MAP_ALLOC
#pragma message(__FILE__": Missing _CRT_MAP_ALLOC, recreate: _CRTDBG_MAP_ALLOC + crtdbg.h")
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#endif

/*---------------------------------------------------------------------------*/

#if NB_HAVE_PB_BOTH==1 && _WIN32 && _MSC_VER

static List markers_map_name = 0;
static List markers_map_data = 0;
static int  mapmarkers_state = 0;

static char *mapmarkers_map_name = 0;

static struct s_full mapmarkers_xf;
static struct s_full mapmarkers_xt;

/*---------------------------------------------------------------------------*/

static std::vector<std::string> splitString(std::string str, const std::string& delimiter)
{
    std::vector<std::string> array;

    size_t pos = 0;
    std::string str_value;

    while ((pos = str.find(delimiter)) != std::string::npos)
    {
        str_value = str.substr(0, pos);
        array.push_back(str_value);
        str.erase(0, pos + delimiter.length());
    }

    array.push_back(str);

    return array;
}

static void removeAllQuotes(const char *source, char *dest)
{
    while (*source)
    {
        if (*source != '"' && *source != '\'')
            *dest++ = *source;

        source++;
    }

    *dest = '\0'; // Null-terminate the destination string
}

/*---------------------------------------------------------------------------*/

static int mapmarkers_load_file(const char *path)
{
    fs_file fh;

    if ((fh = fs_open_read(path)))
    {
        int i, first = 1;
        char *out_line;
        char out_values[3][256];

        while (read_line(&out_line, fh))
        {
            // PHASE 1: COLONS

            std::vector<std::string> out_str_values = splitString(out_line, ";");

            // PHASE 2: QUOTES

            for (i = 0; i < out_str_values.size(); i++)
                removeAllQuotes(out_str_values[i].c_str(), out_values[i]);

            // PHASE 3: SLASHES

            if (out_str_values.size() == 3 && !first)
            {
                struct mapmarker data = {0};
                char map_filename[128];

                data.supported = 1;

                // FILENAME

                strcpy_s(map_filename, 128, out_values[0]);

                if (!(str_ends_with(map_filename, ".sol") || str_ends_with(map_filename, ".solx")))
                    data.supported = 0;

                // LEVEL STATUS

                if (data.supported)
                {
                    if (strcmp(out_values[1], "Failed") == 0 || strcmp(out_values[1], "3") == 0)
                        data.s = 3;
                    else if (strcmp(out_values[1], "Timed-out") == 0 || strcmp(out_values[1], "1") == 0)
                        data.s = 1;
                    else data.supported = 0;
                }

                // POSITION

                if (data.supported)
                {
                    std::vector<std::string> out_str_position = splitString(out_values[2], "/");

                    int pos[3] = { 0, 0, 0 };

                    if (out_str_position.size() != 3)
                        data.supported = 0;
                    else for (i = 0; i < out_str_position.size() && i < 3; i++)
                        pos[i] = atoi(out_str_position[i].c_str());

                    if (data.supported)
                    {
                        data.px = pos[0];
                        data.py = pos[1];
                        data.pz = pos[2];
                    }
                }

                if (data.supported)
                {
                    markers_map_name = list_cons(strdup(map_filename), markers_map_name);
                    markers_map_data = list_cons(NULL, markers_map_data);

                    if (markers_map_data->data = calloc(1, sizeof (data))) {
                        memcpy(markers_map_data->data, &data, sizeof (data));
                    }
                }
            }

            first = 0;
        }

        fs_close(fh);
        log_printf("WGCL Operator: Done!: %s\n", path);
        return 1;
    }
    else log_errorf("WGCL Operator: Can't load the file!: %s / %s\n", path, fs_error());

    return 0;
}

static int mapmarkers_load_csv(void)
{
    char full_path[256];
    sprintf_s(full_path, 256, "%s\\%s\\*", fs_get_write_dir(), "Map Markers");
    WIN32_FIND_DATAA find_data;
    HANDLE hFind;

    if ((hFind = FindFirstFileA(full_path, &find_data)) != INVALID_HANDLE_VALUE)
    {
        do {
            if (strcmp(find_data.cFileName, ".") == 0 ||
                strcmp(find_data.cFileName, "..") == 0)
                continue;

            char tmp_real[256];
            sprintf_s(tmp_real, 256, "%s\\%s", "Map Markers", find_data.cFileName);

            log_printf("WGCL Operator: Loading map marker file...: %s\n", tmp_real);
            mapmarkers_load_file(tmp_real);
        } while (FindNextFileA(hFind, &find_data));

        FindClose(hFind);
        hFind = 0;
        return 1;
    }

    return 0;
}

/*---------------------------------------------------------------------------*/

extern "C" int mapmarkers_init(void)
{
    mapmarkers_quit();

    sol_load_full(&mapmarkers_xf, "geom/opmark/opmark_xf.sol", 0);
    sol_load_full(&mapmarkers_xt, "geom/opmark/opmark_xt.sol", 0);

    if (mapmarkers_load_csv())
        mapmarkers_state = 1;

    return mapmarkers_state;
}

extern "C" void mapmarkers_quit(void)
{
    if (!mapmarkers_state) return;

    mapmarkers_state = 0;

    if (mapmarkers_map_name)
    {
        free(mapmarkers_map_name);
        mapmarkers_map_name = NULL;
    }

    while (markers_map_data)
        markers_map_data = list_rest(markers_map_data);

    while (markers_map_name)
        markers_map_name = list_rest(markers_map_name);

    markers_map_data = 0;
    markers_map_name = 0;

    sol_free_full(&mapmarkers_xt);
    sol_free_full(&mapmarkers_xf);
}

extern "C" void mapmarkers_draw(struct s_rend *rend)
{
    if (!mapmarkers_state || !markers_map_data || !markers_map_name || !config_cheat()) return;

    for (List p1 = markers_map_data, p2 = markers_map_name; p1; ) {
        struct mapmarker *pData = (struct mapmarker *) p1->data;
        char *pDataText = (char *) p2->data;

        if (pData && pDataText && mapmarkers_map_name)
            if (strcmp(pDataText, mapmarkers_map_name) == 0)
            {
                glDisable(GL_DEPTH_TEST);
                glDepthMask(GL_FALSE);
                glPushMatrix();

                if (pData->s == 1 || pData->s == 3)
                    glTranslatef(pData->px / 100.0f, pData->py / 100.0f, pData->pz / 100.0f);

                switch (pData->s) {
                    case 1: sol_draw(&mapmarkers_xt.draw, rend, 1, 1); break;
                    case 3: sol_draw(&mapmarkers_xf.draw, rend, 1, 1); break;
                }

                glPopMatrix();
                glDepthMask(GL_TRUE);
                glEnable(GL_DEPTH_TEST);
            }

        p1 = p1->next;
        p2 = p2->next;
    }
}

extern "C" int mapmarkers_load_map(const char *filename)
{
    if (mapmarkers_map_name)
    {
        free(mapmarkers_map_name);
        mapmarkers_map_name = NULL;
    }

    if (filename) mapmarkers_map_name = strdup(filename);

    return mapmarkers_map_name ? mapmarkers_map_name[0] : 0;
}

/*---------------------------------------------------------------------------*/

#endif
