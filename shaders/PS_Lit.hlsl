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
    if (g_lit_mode == 0)
        return SelectTextureSampler(psInput.uv) * saturate(g_mtrl_diffuse);

    // MTRL settings.
    float4 ambientColor  = saturate(g_mtrl_ambient);
    float4 specularColor = saturate(g_mtrl_specular);
    float  specPow       = g_mtrl_shininess;

    // Lighting settings.
    float3 lightDirection = normalize(g_light_direction.xyz);
    float3 lightColor = 1 - ambientColor;

    // Basic ambient (Ka) and diffuse (Kd) lighting from above.
    float3 N     = psInput.norm;
    float  NdotL = dot     (N, lightDirection);
    float  Ka    = saturate(NdotL + 1);
    float  Kd    = saturate(NdotL);

    // Directional light.
    float3 vec_tmp = lightDirection;

    // Diffuse reflection of light.
    float3 V  = normalize(vec_tmp);
    Kd       += saturate(dot(-V, N)) * saturate(dot(N, lightDirection));

    // Specular lighting.
    float3 R          = reflect(-normalize(lightDirection), normalize(psInput.norm));
    float  specFactor = pow(saturate(dot(R, V)), specPow) * saturate(dot(N, lightDirection));
    float3 specColor  = saturate(specularColor * specFactor);

    // Emmisive.
    float3 emmisiveColor = saturate(g_mtrl_emmisive.rgb);
    
    // Texture diffuse.
    float4 diffuseTexture = SelectTextureSampler(psInput.uv);
    float3 diffuseColor   = diffuseTexture.rgb * lerp((ambientColor.rgb * Ka) + (lightColor * Kd), emmisiveColor, emmisiveColor) * saturate(g_color4.rgb);
    
    // Scene fades.
    float3 diffuseSceneColor = lerp(saturate(diffuseColor + specColor), g_fade_color.rgb, g_fade_color.a);

    // Final.
    return float4(diffuseSceneColor, diffuseTexture.a * g_color4.a);
}