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

PS_Input main(VS_Input_Default input)
{
    PS_Input psOutput;

    psOutput.pos   = mul(float4(input.pos, 1.0f), g_wvp_mtx);
    psOutput.norm  = normalize(mul(float4(input.norm, 0.0f), g_w_mtx).xyz);
    psOutput.color = float4(1, 1, 1, 1);
    psOutput.uv    = input.uv;
    
    //psOutput.lightPos = mul(float4(input.pos, 1.0f), g_wvp_mtx_light);

    return psOutput;
}