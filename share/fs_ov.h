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

#ifndef FS_OV
#define FS_OV

#define OV_EXCLUDE_STATIC_CALLBACKS
#if defined(__WII__)
#include <tremor/ivorbisfile.h>
#else
#include <vorbis/vorbisfile.h>
#endif

//#define fs_ov_seek  fs_seek
//#define fs_ov_close fs_close
//#define fs_ov_tell  fs_tell

size_t fs_ov_read(void* ptr, size_t size, size_t nmemb, void* datasource);
int    fs_ov_seek(void *datasource, ogg_int64_t offset, int whence);
int    fs_ov_close(void *datasource);
long   fs_ov_tell(void *datasource);

#endif
