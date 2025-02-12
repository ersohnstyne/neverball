/*
 * Copyright (C) 2025 Microsoft / Neverball authors / Jānis Rūcis
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

#if !defined(__LEAPMOTION__) && !defined(__HILLCREST_LOOP__)

void tilt_init(void)
{
}

void tilt_free(void)
{
}

int tilt_stat(void)
{
    return 0;
}

int  tilt_get_button(int *b, int *s)
{
    return 0;
}

float tilt_get_x(void)
{
    return 0.0f;
}

float tilt_get_z(void)
{
    return 0.0f;
}

#endif
