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

#ifndef FS_H
#define FS_H

typedef struct fs_file_s *fs_file;

int fs_init(const char *argv0);
int fs_quit(void);

const char *fs_error(void);

const char *fs_base_dir(void);
int         fs_add_path(const char *);
void        fs_remove_path(const char *);
int         fs_add_path_with_archives(const char *);
int         fs_set_write_dir(const char *);
const char *fs_get_write_dir(void);

int fs_recycle(const char *);
int fs_exists(const char *);
int fs_remove(const char *);
int fs_rename(const char *, const char *);

fs_file fs_open_read(const char *);
fs_file fs_open_write(const char *);
fs_file fs_open_append(const char *);

int     fs_close(fs_file);

int  fs_read(void *data, int bytes, fs_file);
int  fs_write(const void *data, int bytes, fs_file);
int  fs_flush(fs_file);
long fs_tell(fs_file);
int  fs_seek(fs_file, long offset, int whence);
int  fs_eof(fs_file);
int  fs_size(const char *);
int  fs_length(fs_file);

int   fs_getc(fs_file);
char *fs_gets(char *dst, int count, fs_file fh);
int   fs_putc(int c, fs_file);
int   fs_puts(const char *src, fs_file);

void *fs_load(const char *path, int *size);

int fs_mkdir(const char *);

#include <stdarg.h>

int fs_printf(fs_file, const char *fmt, ...);

#include "dir.h"
#include "array.h"

Array fs_dir_scan(const char *, int (*filter)(struct dir_item *));
void  fs_dir_free(Array);

const char *fs_resolve(const char *);

void fs_persistent_sync(void);

#endif
