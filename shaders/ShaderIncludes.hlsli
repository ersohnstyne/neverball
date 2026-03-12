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

/*
 * HACK: Used with D3D12 graphics engine, consider preprocessor definitions
 * with VIDEO_DIRECTX12. See documentation, how to get started:
 *
 * https://learn.microsoft.com/en-us/windows/win32/direct3d12/directx-12-programming-guide
 *
 * - Ersohn Styne
 */

cbuffer CB_ModelObject : register(b0)
{
    float4x4 g_w_mtx;
    float4x4 g_wvp_mtx;

    float4 g_light_direction;

    float4 g_mtrl_diffuse;
    float4 g_mtrl_ambient;
    float4 g_mtrl_specular;
    float4 g_mtrl_emmisive;
    float  g_mtrl_shininess;

    float4 g_color4;
    float4 g_fade_color;

    int g_texture_mode;
    int g_lit_mode;
};

Texture2D g_texture0 : register(t0);

SamplerState g_sampler0 : register(s0);
SamplerState g_sampler1 : register(s1);
SamplerState g_sampler2 : register(s2);
SamplerState g_sampler3 : register(s3);

/*****************************************************************************/

struct VS_Input_Default
{
    float3 pos  : Position;
    float3 norm : Normal;
    float2 uv   : Texcoord;
};

struct VS_Input_UI
{
    float2 pos   : Position;
    float4 color : Color;
    float2 uv    : Texcoord;
};

struct PS_Input_UI
{
    float4 pos   : SV_Position;
    float4 color : Color0;
    float2 uv    : Texcoord0;
};

struct PS_Input
{
    float4 pos   : SV_Position;
    float3 norm  : Normal;
    float4 color : Color0;
    float2 uv    : Texcoord0;

    float4 lightPos : Texcoord10;
};

float4 SelectTextureSampler(float2 inUV)
{
    if (g_texture_mode > -1)
    {
        if (g_texture_mode == 1)
            return g_texture0.Sample(g_sampler1, inUV);
        else if (g_texture_mode == 2)
            return g_texture0.Sample(g_sampler2, inUV);
        else if (g_texture_mode == 3)
            return g_texture0.Sample(g_sampler3, inUV);
        else return g_texture0.Sample(g_sampler0, inUV);
    }
    else return float4(1, 1, 1, 1);
}
