/*
 * Copyright (C) 2026 Ersohn Styne
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

#include "ShaderIncludes.hlsli"

PS_Input_UI main(VS_Input_UI input)
{
    PS_Input_UI psOutput;

    psOutput.pos = mul(float4(input.pos.xy, 0, 1), g_wvp_mtx);

    // An Pixelshader weitergeleitet.
    psOutput.color = input.color;
    psOutput.uv    = input.uv;

    return psOutput;
}