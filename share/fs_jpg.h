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

#ifndef FS_JPG
#define FS_JPG

/*
 * jpeglib.h triggers errors due to missing size_t and FILE type
 * definitions.  Include these two headers as a workaround.
 */

#include <stddef.h>
#include <stdio.h>
#include <jpeglib.h>

#include "fs.h"

void fs_jpg_src(j_decompress_ptr cinfo, fs_file infile);

#endif
