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

float4 main(PS_Input psInput) : SV_Target
{
    float4 diffuseTexture = SelectTextureSampler(psInput.uv);
    float3 diffuseColor = diffuseTexture.rgb * psInput.color.rgb * g_color4.rgb;

    return float4(saturate(diffuseColor), saturate(diffuseTexture.a * psInput.color.a * g_color4.a));
}