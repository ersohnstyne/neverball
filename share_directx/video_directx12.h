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

#ifndef VIDEO_DIRECTX12_H
#define VIDEO_DIRECTX12_H

#define VIDEO_D3D12_MAX_FRAME_COUNT 3

using namespace Microsoft::WRL;

/* === DX12 DECLARATIONS === */

#define DEFINE_CBV_STRUCT(_packed, _name) \
    struct _name _packed; \
    static_assert(sizeof (_name) <= 256, L#_name L": Mehr als 256 byte im Konstantenpuffer!")

#define DEFINE_CBV_STRUCT_X2(_packed, _name) \
    struct _name _packed; \
    static_assert(sizeof (_name) <= 512, L#_name L": Mehr als 512 byte im Konstantenpuffer!")

#define CBV_ALIGNED_SIZE(_struct_name) \
    static constexpr UINT c_aligned_size_##_struct_name = \
        (sizeof (##_struct_name) + 256) & ~256

DEFINE_CBV_STRUCT({
    DirectX::XMFLOAT4X4 g_w_mtx;
    DirectX::XMFLOAT4X4 g_wvp_mtx;

    DirectX::XMFLOAT4 g_light_direction;

    DirectX::XMFLOAT4 g_mtrl_diffuse;
    DirectX::XMFLOAT4 g_mtrl_ambient;
    DirectX::XMFLOAT4 g_mtrl_specular;
    DirectX::XMFLOAT4 g_mtrl_emmisive;
    float             g_mtrl_shininess;

    DirectX::XMFLOAT4 g_color4;
    DirectX::XMFLOAT4 g_fade_color;

    int g_texture_mode;
    int g_lit_mode;

    float padding[1];
}, CB_ModelObject);

struct _D3D12_RESOURCE_BUFFER_OBJECT {
    Microsoft::WRL::ComPtr<ID3D12Resource2> m_objectResource;
    D3D12_RESOURCE_STATES                   m_objectResourceState;
};

typedef struct _D3D12_RESOURCE_BUFFER_OBJECT D3D12_RESOURCE_BUFFER_OBJECT;

/* === END DX12 DECLARATIONS === */

extern ComPtr<ID3D12GraphicsCommandList2> g_cmdList;

/*---------------------------------------------------------------------------*/

int  video_directx12_init(int, int, int);
void video_directx12_quit(void);

/*---------------------------------------------------------------------------*/

/* Use video_directx12_init instead of video_directx12_mode. */
#define video_directx12_mode video_directx12_init

BOOL video_directx12_swap(void);

void video_directx12_show_cursor(void);
void video_directx12_hide_cursor(void);

void video_directx12_fullscreen(int);
BOOL video_directx12_resize(int, int);
void video_directx12_set_window_size(int, int);
void video_directx12_set_display(int);
int  video_directx12_display(void);

/*---------------------------------------------------------------------------*/

void video_directx12_set_perspective(float, float, float);
void video_directx12_set_ortho(void);

void video_directx12_prepare_srv(const int);
void video_directx12_prepare_cbv(const int, const int, const int);

/*---------------------------------------------------------------------------*/

BOOL video_directx12_prepare_frame(void);
BOOL video_directx12_clear(void);

/*---------------------------------------------------------------------------*/

/*
 * Exercise extreme caution, when assuming DirectX native linkage for Windows.
 */

HRESULT directx12_next_resource_state(ID3D12Resource2 *,
                                      D3D12_RESOURCE_STATES *,
                                      D3D12_RESOURCE_STATES);

HRESULT directx12_create_buffer(UINT, D3D12_RESOURCE_BUFFER_OBJECT *);
void    directx12_delete_buffer(UINT, D3D12_RESOURCE_BUFFER_OBJECT *);
HRESULT directx12_set_buffer_data(D3D12_RESOURCE_BUFFER_OBJECT, long, PVOID);
HRESULT directx12_set_buffer_subdata(D3D12_RESOURCE_BUFFER_OBJECT, long, long, PVOID);

void directx12_push_matrix_modelview(void);
void directx12_pop_matrix_modelview (void);

void directx12_matrix_translatef(float x, float y, float z);
void directx12_matrix_translatef(float pos[3]);

void directx12_matrix_scalef(float x, float y, float z);
void directx12_matrix_scalef(float multiply[3]);

void directx12_matrix_rotatef(float angle, float x, float y, float z);
void directx12_matrix_rotatef(float angle, float vaxis[3]);

void directx12_matrix_set     (float m[16]);
void directx12_matrix_multiply(float m[16]);

void directx12_color4ub(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
void directx12_color4ub(unsigned char vcolor[4]);

void directx12_color4f(float r, float g, float b, float a);
void directx12_color4f(float vcolor[4]);

void directx12_set_viewport(int x, int y, int w, int h);
void directx12_set_scissor (int x, int y, int w, int h);
void directx12_reset_scissor(void);

/*---------------------------------------------------------------------------*/

#endif