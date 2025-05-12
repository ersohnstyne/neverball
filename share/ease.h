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

#ifndef EASE_H
#define EASE_H 1

float easeInBack(float x);
float easeOutBack(float x);
float easeInOutBack(float x);
float easeInElastic(float x);
float easeOutElastic(float x);
float easeInOutElastic(float x);
float easeInBounce(float x);
float easeOutBounce(float x);
float easeInOutBounce(float x);

#endif
