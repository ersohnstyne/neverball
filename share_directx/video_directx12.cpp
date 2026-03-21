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
 * HACK: Remembering the code file differences:
 * Developers  who  programming  C++  can see more bedrock declaration
 * than C.  Developers  who  programming  C  can  see  few  procedural
 * declaration than  C++.  Keep  in  mind  when making  sure that your
 * extern code must associated. The valid file types are *.c and *.cpp,
 * so it's always best when making cross C++ compiler to keep both.
 * - Ersohn Styne
 */

#if _WIN32
#ifndef __cplusplus
#pragma message(__FILE__": ERROR: No C++ detected for DX12!")
#endif

#ifndef _MSC_VER
#pragma message(__FILE__": ERROR: Only Visual Studio for Windows to be compiled with it!")
#elif _MSC_VER < 1950
#pragma message(__FILE__": ERROR: This project requires MSVC++ 14.50 or newer!")
#endif

#define NOMINMAX
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <lmcons.h>

#if _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

//#include <wrl.h>
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi.h>
#include <dxgi1_6.h>

#include "d3dx12.h"

#ifdef _DEBUG
#include <d3d12sdklayers.h>
#include <dxgidebug.h>
#endif

#include <DirectXMath.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

#define Ensure(condition) \
    do { \
        if (!(condition)) { \
            OutputDebugStringA("[!] Debug Error: Ensure condition failed! \n    Expression: " #condition ## "\n    File: " __FILE__ "\n    Line: " _CRT_STRINGIZE(__LINE__) "\n"); \
            __debugbreak(); \
            return false; \
        } \
    } while (false)

#define Ensure_HRESULT(condition) \
    do { \
        if (FAILED(condition)) { \
            OutputDebugStringA("[!] Debug HR Error: Ensure condition failed! \n    HR Expression: " #condition ## "\n    File: " __FILE__ "\n    Line: " _CRT_STRINGIZE(__LINE__) "\n"); \
            __debugbreak(); \
            return false; \
        } \
    } while (false)
#endif

extern "C" {
#include "video.h"
#include "video_directx12.h"

#include "fs.h"
#include "vec3.h"
#include "config.h"
#include "gui.h"
#include "log.h"
}

/*
 * HACK: Used with D3D12 graphics engine, consider preprocessor definitions
 * with VIDEO_DIRECTX12. See documentation, how to get started:
 *
 * https://learn.microsoft.com/en-us/windows/win32/direct3d12/directx-12-programming-guide
 *
 * - Ersohn Styne
 */

#if _WIN32

/*---------------------------------------------------------------------------*/

extern "C" void video_directx12_show_cursor(void)
{
    gui_set_cursor(1);
}

extern "C" void video_directx12_hide_cursor(void)
{
    gui_set_cursor(0);
}

/*---------------------------------------------------------------------------*/

/* === WIN32 MEMBER VARIABLES === */

static BOOL g_windowInited = FALSE;

static HINSTANCE g_hInstance        = 0;
static ATOM      g_wndClass         = 0;
static BOOL      g_windowFullscreen = FALSE;
static HWND      g_hWnd             = 0;

/* === END WIN32 MEMBER VARIABLES === */

/* === DX12 CLASS MEMBERS === */

class WIN_ShaderFile
{
public:
    WIN_ShaderFile(const char* FilePath)
    {
        fs_file shader_in = fs_open_read(FilePath);

        if (shader_in)
        {
            fs_seek(shader_in, 0, SEEK_END);
            m_size = fs_tell(shader_in);
            fs_seek(shader_in, 0, SEEK_SET);
            m_data = malloc(m_size);

            if (m_data) fs_read(m_data, m_size, shader_in);

            fs_close(shader_in);
        }
    }

    ~WIN_ShaderFile()
    {
        m_size = 0;

        if (m_data) {
            free(m_data);
            m_data = NULL;
        }
    }

    inline const void  *GetBuffer() const { return m_data; }
    inline const size_t GetSize()   const { return m_size; }

private:
    PVOID  m_data = nullptr;
    size_t m_size = 0;
};

/* === END DX12 CLASS MEMBERS === */

/* === DX12 MEMBER VARIABLES === */

static int g_mat_modelview_depthRemaining;
static DirectX::XMMATRIX g_mat_projection;
static DirectX::XMMATRIX g_mat_modelview[32];

CBV_ALIGNED_SIZE(CB_ModelObject);
CB_ModelObject  g_cbModelObjectData;
UINT8          *g_mappedCbModelObject[VIDEO_D3D12_MAX_FRAME_COUNT];

static BOOL g_dx12Inited       = FALSE;
static BOOL g_renderViewInited = FALSE;
static BOOL g_framePrepared    = FALSE;

#ifdef _DEBUG
bool m_debugLayerEnabled = false;

ComPtr<ID3D12Debug6> g_d3d12Debug;
ComPtr<IDXGIDebug1>  g_dxgiDebug;
#endif

ComPtr<IDXGIFactory7>              g_dxgiFactory;
ComPtr<IDXGIAdapter4>              g_dxgiHwAdapter;
ComPtr<IDXGIAdapter4>              g_dxgiWarpAdapter;
ComPtr<ID3D12Device14>             g_d3d12Device;
ComPtr<ID3D12InfoQueue>            g_infoQueue;
ComPtr<ID3D12CommandQueue>         g_cmdQueue;
ComPtr<IDXGISwapChain4>            g_swapChain;
ComPtr<ID3D12DescriptorHeap>       g_rtvHeap, g_dsvHeap;
ComPtr<ID3D12CommandAllocator>     g_cmdAllocators[VIDEO_D3D12_MAX_FRAME_COUNT];
ComPtr<ID3D12RootSignature>        g_rootSignature;
ComPtr<ID3D12PipelineState>        g_psoDefault, g_psoReflective, g_psoDoubleSided, g_psoUserInterface;
ComPtr<ID3D12GraphicsCommandList2> g_cmdList;
ComPtr<ID3D12Fence>                g_fence;

ComPtr<ID3D12Resource2> g_renderTargets[VIDEO_D3D12_MAX_FRAME_COUNT], g_depthStencil, g_cbvModelObject[VIDEO_D3D12_MAX_FRAME_COUNT];
D3D12_RESOURCE_STATES   g_renderTargetStates[VIDEO_D3D12_MAX_FRAME_COUNT], g_depthStencilState, g_cbvModelObjectStates[VIDEO_D3D12_MAX_FRAME_COUNT];

ComPtr<ID3D12DescriptorHeap> g_msaaRtvHeap, g_msaaDsvHeap;
ComPtr<ID3D12Resource2>      g_msaaRenderTarget, g_msaaDepthStencil;
D3D12_RESOURCE_STATES        g_msaaRenderTargetState, g_msaaDepthStencilState;

UINT                        g_rtvDescriptorSize;
D3D12_CPU_DESCRIPTOR_HANDLE g_rtvHandles[VIDEO_D3D12_MAX_FRAME_COUNT];
UINT64                      g_fenceValues[VIDEO_D3D12_MAX_FRAME_COUNT];
HANDLE                      g_fenceEvent;

UINT g_currentFrame = 0, g_currentCbvIndex = 0, g_descHeapMode = 0, g_max_samples = 16;

/* === END DX12 MEMBER VARIABLES === */

/* === DX12 FUNCTIONS === */

static D3D12_STATIC_SAMPLER_DESC InitStaticSamplerDesc(
    UINT ShaderRegister,
    D3D12_FILTER Filter = D3D12_FILTER_MIN_MAG_MIP_POINT,
    bool ClampU = false, bool ClampV = false, bool Border = false)
{
    D3D12_STATIC_SAMPLER_DESC ssDesc {};

    ssDesc.Filter           = Filter;
    ssDesc.AddressU         = Border ? D3D12_TEXTURE_ADDRESS_MODE_BORDER : ClampU ? D3D12_TEXTURE_ADDRESS_MODE_CLAMP : D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    ssDesc.AddressV         = Border ? D3D12_TEXTURE_ADDRESS_MODE_BORDER : ClampV ? D3D12_TEXTURE_ADDRESS_MODE_CLAMP : D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    ssDesc.AddressW         = Border ? D3D12_TEXTURE_ADDRESS_MODE_BORDER : D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    ssDesc.MipLODBias       = 0;
    ssDesc.MaxAnisotropy    = 1;
    ssDesc.ComparisonFunc   = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    ssDesc.BorderColor      = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    ssDesc.MinLOD           = 0.0f;
    ssDesc.MaxLOD           = D3D12_FLOAT32_MAX;
    ssDesc.ShaderRegister   = ShaderRegister;
    ssDesc.RegisterSpace    = 0;
    ssDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    return ssDesc;
}

static D3D12_GRAPHICS_PIPELINE_STATE_DESC InitPSODesc(
    D3D12_INPUT_ELEMENT_DESC pInput[], UINT inputSize,
    ID3D12RootSignature* pRootSig,
    D3D12_SHADER_BYTECODE VS, D3D12_SHADER_BYTECODE PS,
    DXGI_FORMAT RTFormat, DXGI_FORMAT DSFormat, bool Reflective, bool DoubleSided = false, bool Undepth = false, UINT sampleCount = 1)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC out_psoDesc = {0};

    out_psoDesc.pRootSignature        = pRootSig;
    out_psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    out_psoDesc.InputLayout = { pInput, inputSize };
    out_psoDesc.VS          = VS;
    out_psoDesc.PS          = PS;

    out_psoDesc.NumRenderTargets = 1;
    out_psoDesc.RTVFormats[0]    = RTFormat;
    out_psoDesc.DSVFormat        = DSFormat;

    out_psoDesc.BlendState.RenderTarget[0] = {
        TRUE, FALSE,
        D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL
    };

    out_psoDesc.BlendState.AlphaToCoverageEnable  = FALSE;
    out_psoDesc.BlendState.IndependentBlendEnable = FALSE;
    
    out_psoDesc.DepthStencilState.DepthEnable      = !Undepth && !Reflective;
    out_psoDesc.DepthStencilState.DepthFunc        = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    out_psoDesc.DepthStencilState.DepthWriteMask   = D3D12_DEPTH_WRITE_MASK_ALL;
    out_psoDesc.DepthStencilState.StencilEnable    = !Undepth && Reflective;
    out_psoDesc.DepthStencilState.StencilReadMask  = 0xFF;
    out_psoDesc.DepthStencilState.StencilWriteMask = 0xFF;

    out_psoDesc.DepthStencilState.BackFace = {
        D3D12_STENCIL_OP_DECR,
        D3D12_STENCIL_OP_DECR,
        D3D12_STENCIL_OP_INCR,
        D3D12_COMPARISON_FUNC_ALWAYS
    };

    out_psoDesc.DepthStencilState.FrontFace = {
        D3D12_STENCIL_OP_KEEP,
        D3D12_STENCIL_OP_KEEP,
        D3D12_STENCIL_OP_KEEP,
        D3D12_COMPARISON_FUNC_EQUAL
    };

    out_psoDesc.SampleMask = UINT_MAX;
    out_psoDesc.SampleDesc = { sampleCount > 1 ? sampleCount : 1, 0 };

    D3D12_FILL_MODE vertex_fill_mode = D3D12_FILL_MODE_SOLID;

    out_psoDesc.RasterizerState = {
        vertex_fill_mode, DoubleSided ? D3D12_CULL_MODE_NONE : Reflective ? D3D12_CULL_MODE_FRONT : D3D12_CULL_MODE_BACK,
        TRUE, 0, .0f, .0f, FALSE, FALSE, FALSE, 0
    };
    return out_psoDesc;
}

static HRESULT NextResourceState(ID3D12Resource* pResource, D3D12_RESOURCE_STATES* CurrentState, D3D12_RESOURCE_STATES NextState)
{
    if (!CurrentState) return E_INVALIDARG;

    if (*CurrentState != NextState) {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(pResource, *CurrentState, NextState);
        g_cmdList->ResourceBarrier(1, &barrier);

        if (CurrentState) *CurrentState = NextState;
    }

    return S_OK;
}

static bool UpdateRenderViews()
{
    if (!g_dx12Inited) {
        log_errorf("DX12: Not initialized!");
        return true;
    }

    for (UINT i = 0; i < 32; i++)
        g_mat_modelview[i] = DirectX::XMMatrixIdentity();

    g_mat_projection = DirectX::XMMatrixIdentity();

    g_mat_modelview_depthRemaining = 31;

    if (config_get_d(CONFIG_MULTISAMPLE) > g_max_samples)
        config_set_d(CONFIG_MULTISAMPLE, g_max_samples);

    RECT gameRect;
    GetClientRect(g_hWnd, &gameRect);

    // Reset and create RTV

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(g_rtvHeap->GetCPUDescriptorHandleForHeapStart());

    for (UINT i = 0; i < VIDEO_D3D12_MAX_FRAME_COUNT; i++) {
        if (g_renderTargets[i] != nullptr)
            g_renderTargets[i].Reset();

        Ensure_HRESULT(g_swapChain->GetBuffer(i, IID_PPV_ARGS(&g_renderTargets[i])));

        D3D12_RENDER_TARGET_VIEW_DESC rtv{};
        rtv.Format               = DXGI_FORMAT_B8G8R8A8_UNORM;
        rtv.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;
        rtv.Texture2D.MipSlice   = 0;
        rtv.Texture2D.PlaneSlice = 0;

        g_d3d12Device->CreateRenderTargetView(g_renderTargets[i].Get(), &rtv, rtvHandle);

        g_renderTargetStates[i] = D3D12_RESOURCE_STATE_PRESENT;

        rtvHandle.Offset(g_rtvDescriptorSize);
    }

    // === MSAA Render Target Init ===

    if (g_msaaRenderTarget != nullptr)
        g_msaaRenderTarget.Reset();

    if (config_get_d(CONFIG_MULTISAMPLE) > 1) {
        D3D12_HEAP_PROPERTIES rtHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

        D3D12_RESOURCE_DESC msaaRTDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_B8G8R8A8_UNORM, static_cast<UINT>(gameRect.right), static_cast<UINT>(gameRect.bottom), 1, 1, config_get_d(CONFIG_MULTISAMPLE) > 1 ? config_get_d(CONFIG_MULTISAMPLE) : 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

        const float c_clearColor[4] = { 0, 0, 0, 0 };
        D3D12_CLEAR_VALUE msaaClearValue = {};
        msaaClearValue.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        memcpy(msaaClearValue.Color, c_clearColor, sizeof(float) * 4);

        g_msaaRenderTargetState = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
        Ensure_HRESULT(g_d3d12Device->CreateCommittedResource(
            &rtHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &msaaRTDesc,
            g_msaaRenderTargetState,
            &msaaClearValue,
            IID_PPV_ARGS(&g_msaaRenderTarget)));

        D3D12_RENDER_TARGET_VIEW_DESC msaaRtv = {};
        msaaRtv.Format               = DXGI_FORMAT_B8G8R8A8_UNORM;
        msaaRtv.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2DMS;
        msaaRtv.Texture2D.MipSlice   = 0;
        msaaRtv.Texture2D.PlaneSlice = 0;

        g_d3d12Device->CreateRenderTargetView(g_msaaRenderTarget.Get(), &msaaRtv, g_msaaRtvHeap->GetCPUDescriptorHandleForHeapStart());
    }

    // === End MSAA Render Target Init ===

    D3D12_HEAP_PROPERTIES depthHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    D3D12_RESOURCE_DESC depthResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R24G8_TYPELESS, static_cast<UINT>(gameRect.right), static_cast<UINT>(gameRect.bottom), 1, 1, config_get_d(CONFIG_MULTISAMPLE) > 1 ? config_get_d(CONFIG_MULTISAMPLE) : 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    CD3DX12_CLEAR_VALUE depthOptimizedClearValue(DXGI_FORMAT_D24_UNORM_S8_UINT, 1.f, 0);

    // Reset and create DSV

    if (g_depthStencil != nullptr)
        g_depthStencil.Reset();

    g_depthStencilState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    Ensure_HRESULT(g_d3d12Device->CreateCommittedResource(
        &depthHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &depthResourceDesc,
        g_depthStencilState,
        &depthOptimizedClearValue,
        IID_PPV_ARGS(&g_depthStencil)));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
    dsv.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsv.ViewDimension      = config_get_d(CONFIG_MULTISAMPLE) > 1 ? D3D12_DSV_DIMENSION_TEXTURE2DMS : D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv.Texture2D.MipSlice = 0;
    dsv.Flags              = D3D12_DSV_FLAG_NONE;
    g_d3d12Device->CreateDepthStencilView(g_depthStencil.Get(), &dsv, g_dsvHeap->GetCPUDescriptorHandleForHeapStart());

    // === MSAA Depth Stencil Init ===

    if (config_get_d(CONFIG_MULTISAMPLE) > 1) {
        if (g_msaaDepthStencil != nullptr)
            g_msaaDepthStencil.Reset();

        D3D12_RESOURCE_DESC msaaDepthResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R24G8_TYPELESS, static_cast<UINT>(gameRect.right), static_cast<UINT>(gameRect.bottom), 1, 1, config_get_d(CONFIG_MULTISAMPLE) > 1 ? config_get_d(CONFIG_MULTISAMPLE) : 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

        g_msaaDepthStencilState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        g_d3d12Device->CreateCommittedResource(
            &depthHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &depthResourceDesc,
            g_msaaDepthStencilState,
            &depthOptimizedClearValue,
            IID_PPV_ARGS(&g_msaaDepthStencil));

        D3D12_DEPTH_STENCIL_VIEW_DESC msaaDsv = {};
        msaaDsv.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
        msaaDsv.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2DMS;
        msaaDsv.Texture2D.MipSlice = 0;
        msaaDsv.Flags              = D3D12_DSV_FLAG_NONE;
        g_d3d12Device->CreateDepthStencilView(g_msaaDepthStencil.Get(), &msaaDsv, g_msaaDsvHeap->GetCPUDescriptorHandleForHeapStart());
    }

    // === MSAA Depth Stencil Init ===

    g_renderViewInited = true;

    return true;
}

static bool WaitForGPU()
{
    if (!g_dx12Inited) {
        log_errorf("DX12: Not initialized!");
        return true;
    }

    Ensure_HRESULT(g_cmdQueue->Signal(g_fence.Get(), g_fenceValues[g_currentFrame]));

    Ensure_HRESULT(g_fence->SetEventOnCompletion(g_fenceValues[g_currentFrame], g_fenceEvent));
    WaitForSingleObjectEx(g_fenceEvent, INFINITE, FALSE);

    g_fenceValues[g_currentFrame]++;

    return true;
}

static bool NextFrame()
{
    if (!g_dx12Inited) {
        log_errorf("DX12: Not initialized!");
        return true;
    }

    const UINT64 currentFenceValue = g_fenceValues[g_currentFrame];
    Ensure_HRESULT(g_cmdQueue->Signal(g_fence.Get(), currentFenceValue));

    g_currentFrame = g_swapChain->GetCurrentBackBufferIndex();

    if (g_fence->GetCompletedValue() < g_fenceValues[g_currentFrame]) {
        Ensure_HRESULT(g_fence->SetEventOnCompletion(g_fenceValues[g_currentFrame], g_fenceEvent));
        WaitForSingleObjectEx(g_fenceEvent, INFINITE, FALSE);
    }

    g_fenceValues[g_currentFrame] = currentFenceValue + 1;

    return true;
}

static bool Flush()
{
    if (!g_dx12Inited) {
        log_errorf("DX12: Not initialized!");
        return true;
    }

    for (UINT i = 0; i < VIDEO_D3D12_MAX_FRAME_COUNT; i++)
        Ensure(WaitForGPU());

    return true;
}

/* === END DX12 FUNCTIONS === */

/*---------------------------------------------------------------------------*/

extern "C" void video_directx12_fullscreen(int f)
{
    DWORD style   = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    DWORD exStyle = WS_EX_WINDOWEDGE | WS_EX_APPWINDOW;

    if (f) {
        style   = WS_POPUP | WS_VISIBLE;
        exStyle = WS_EX_APPWINDOW;
    }

    SetWindowLong(g_hWnd, GWL_STYLE,   style);
    SetWindowLong(g_hWnd, GWL_EXSTYLE, exStyle);

    if (f) {
        const HMONITOR monitor = MonitorFromWindow(g_hWnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO monitorInfo {};
        monitorInfo.cbSize = sizeof (monitorInfo);
        if (GetMonitorInfo(monitor, &monitorInfo)) {
            SetWindowPos(g_hWnd, nullptr,
                monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.top,
                monitorInfo.rcMonitor.right  - monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }

        RECT clientRect { monitorInfo.rcMonitor.left,
                          monitorInfo.rcMonitor.top,
                          monitorInfo.rcMonitor.right  - monitorInfo.rcMonitor.left,
                          monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top };
        GetClientRect(g_hWnd, &clientRect);

        POINT centerPoint { monitorInfo.rcMonitor.left + (clientRect.right  - clientRect.left) / 2,
                            monitorInfo.rcMonitor.top  + (clientRect.bottom - clientRect.top)  / 2 };
        SetCursorPos(centerPoint.x, centerPoint.y);
    } else {
        ShowWindow(g_hWnd, SW_MAXIMIZE);

        RECT wnd_rect, clientRect;
        GetWindowRect(g_hWnd, &wnd_rect);
        GetClientRect(g_hWnd, &clientRect);
        
        POINT centerPoint { wnd_rect.left + (wnd_rect.right  - wnd_rect.left) / 2,
                            wnd_rect.top  + (wnd_rect.bottom - wnd_rect.top)  / 2 };

        SetCursorPos(centerPoint.x, centerPoint.y);
    }

    g_windowFullscreen = f;
    config_set_d(CONFIG_FULLSCREEN, g_windowFullscreen ? 1 : 0);
}

extern "C" BOOL video_directx12_resize(int window_w, int window_h)
{
    RECT wsz_rect, wsz_clientrect;

    /* Update window size (for mouse events). */

    GetWindowRect(g_hWnd, &wsz_rect);
    video.window_w = wsz_rect.left - (wsz_rect.right  - wsz_rect.left);
    video.window_h = wsz_rect.top  - (wsz_rect.bottom - wsz_rect.top);

    /* User experience thing: don't save fullscreen size to config. */

    if (!config_get_d(CONFIG_FULLSCREEN))
    {
        config_set_d(CONFIG_WIDTH,  video.window_w);
        config_set_d(CONFIG_HEIGHT, video.window_h);
    }

    /* Update drawable size (for rendering). */

    GetClientRect(g_hWnd, &wsz_clientrect);
    video.device_w = wsz_clientrect.right;
    video.device_h = wsz_clientrect.bottom;

    video.scale_w = floorf((float) window_w / (float) video.device_w);
    video.scale_h = floorf((float) window_h / (float) video.device_h);

    video.device_scale = (float) video.device_h / (float) video.window_h;

    if (!g_dx12Inited) {
        log_errorf("DX12: Not initialized!");
        return true;
    }

    Ensure(Flush());

    for (UINT i = 0; i < VIDEO_D3D12_MAX_FRAME_COUNT; i++) {
        if (g_renderTargets[i] != nullptr) {
            g_renderTargets[i].Reset();
            g_fenceValues[i] = g_fenceValues[g_currentFrame];
        }
    }

    if (config_get_d(CONFIG_MULTISAMPLE) > 1) {
        if (g_msaaRenderTarget != nullptr) g_msaaRenderTarget.Reset();
    }

    if (g_depthStencil != nullptr) g_depthStencil.Reset();

    DXGI_SWAP_CHAIN_DESC scd = { 0 };
    Ensure_HRESULT(g_swapChain->GetDesc(&scd));
    Ensure_HRESULT(g_swapChain->ResizeBuffers(VIDEO_D3D12_MAX_FRAME_COUNT, window_w, window_h, scd.BufferDesc.Format, scd.Flags));

    g_currentFrame = g_swapChain->GetCurrentBackBufferIndex();

    return TRUE;
}

extern "C" void video_directx12_set_window_size(int w, int h)
{
    POINT pos { 0, 0 };
    GetCursorPos(&pos);
    HMONITOR    monitor = MonitorFromPoint(pos, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO monitorInfo {};
    monitorInfo.cbSize = sizeof (monitorInfo);
    GetMonitorInfo(monitor, &monitorInfo);

    RECT centerRect = { monitorInfo.rcWork.left + ((monitorInfo.rcWork.right  - monitorInfo.rcWork.left) / 2l),
                        monitorInfo.rcWork.top  + ((monitorInfo.rcWork.bottom - monitorInfo.rcWork.top)  / 2l),
                        0,
                        0 };

    UINT defaultWidth  = w;
    UINT defaultHeight = h;

    bool bFullscreenDetected = (monitorInfo.rcMonitor.right  - monitorInfo.rcMonitor.left == defaultWidth &&
                                monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top  == defaultHeight);

    RECT gameRect = { static_cast<long>(centerRect.left - (defaultWidth  / 2l)),
                      static_cast<long>(centerRect.top  - (defaultHeight / 2l)),
                      static_cast<long>(centerRect.left + (defaultWidth  / 2l)),
                      static_cast<long>(centerRect.top  + (defaultHeight / 2l)) };

    AdjustWindowRect(&gameRect,
                     bFullscreenDetected ? WS_POPUP | WS_VISIBLE : WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                     FALSE);

    SetWindowPos(g_hWnd, nullptr,
        gameRect.left, gameRect.top,
        gameRect.right - gameRect.left, gameRect.bottom - gameRect.top,
        SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
}

extern "C" void video_directx12_set_display(int dpy)
{
    UINT displayCount = 0;
    static RECT displayRects[9]{
        {0,0,0,0},
        {0,0,0,0},
        {0,0,0,0},
        {0,0,0,0},
        {0,0,0,0},
        {0,0,0,0},
        {0,0,0,0},
        {0,0,0,0},
        {0,0,0,0}
    };

    EnumDisplayMonitors(nullptr, nullptr,
        [](HMONITOR hMonitor, HDC, LPRECT pRect, LPARAM lParam) -> BOOL
        {
            if (!lParam || *(UINT *) lParam >= 9 || !pRect)
                return FALSE;

            displayRects[*(UINT *) lParam] = *pRect;

            *(UINT *) lParam += 1;

            return TRUE;
        },
        (LPARAM) &displayCount);

    UINT displayIndex = config_get_d(CONFIG_DISPLAY) > displayCount - 1 ? displayCount - 1 : config_get_d(CONFIG_DISPLAY);

    RECT centerRect = { displayRects[displayIndex].left + ((displayRects[displayIndex].right  - displayRects[displayIndex].left) / 2),
                        displayRects[displayIndex].top  + ((displayRects[displayIndex].bottom - displayRects[displayIndex].top)  / 2),
                        0,
                        0 };
    
    UINT defaultWidth  = config_get_d(CONFIG_WIDTH);
    UINT defaultHeight = config_get_d(CONFIG_HEIGHT);

    bool bFullscreenDetected = (displayRects[displayIndex].right  - displayRects[displayIndex].left == defaultWidth &&
                                displayRects[displayIndex].bottom - displayRects[displayIndex].top  == defaultHeight);

    RECT gameRect = { static_cast<long>(centerRect.left - (defaultWidth  / 2l)),
                      static_cast<long>(centerRect.top  - (defaultHeight / 2l)),
                      static_cast<long>(centerRect.left + (defaultWidth  / 2l)),
                      static_cast<long>(centerRect.top  + (defaultHeight / 2l)) };

    AdjustWindowRect(&gameRect,
                     bFullscreenDetected ? WS_POPUP | WS_VISIBLE : WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                     FALSE);

    SetWindowPos(g_hWnd, nullptr,
        gameRect.left, gameRect.top,
        gameRect.right - gameRect.left, gameRect.bottom - gameRect.top,
        SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

    if (bFullscreenDetected) video_directx12_fullscreen(1);
}

extern "C" int video_directx12_display(void)
{
    UINT displayCount = 0;
    static RECT displayRects[9]{
        {0,0,0,0},
        {0,0,0,0},
        {0,0,0,0},
        {0,0,0,0},
        {0,0,0,0},
        {0,0,0,0},
        {0,0,0,0},
        {0,0,0,0},
        {0,0,0,0}
    };

    EnumDisplayMonitors(nullptr, nullptr,
        [](HMONITOR hMonitor, HDC, LPRECT pRect, LPARAM lParam) -> BOOL
        {
            if (!lParam || *(UINT *) lParam >= 9 || !pRect)
                return FALSE;

            displayRects[*(UINT *) lParam] = *pRect;

            *(UINT *) lParam += 1;

            return TRUE;
        },
        (LPARAM) &displayCount);

    RECT gameRect { 0, 0, config_get_d(CONFIG_WIDTH), config_get_d(CONFIG_HEIGHT) };
    GetWindowRect(g_hWnd, &gameRect);

    POINT pos { gameRect.left + (gameRect.right  - gameRect.left) / 2,
                gameRect.top  + (gameRect.bottom - gameRect.top)  / 2 };

    HMONITOR    monitor = MonitorFromPoint(pos, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO monitorInfo {};
    monitorInfo.cbSize = sizeof(monitorInfo);
    GetMonitorInfo(monitor, &monitorInfo);

    for (int i = 0; i < displayCount; i++)
        if (monitorInfo.rcMonitor.left < pos.x && pos.x > monitorInfo.rcMonitor.left + (monitorInfo.rcMonitor.right  - monitorInfo.rcMonitor.left) &&
            monitorInfo.rcMonitor.top  < pos.y && pos.y > monitorInfo.rcMonitor.top  + (monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top))
            return i;

    return -1;
}

/*---------------------------------------------------------------------------*/

extern "C" int video_directx12_init(int f, int w, int h)
{
    WNDCLASSEX wcex {};

    if (!g_hInstance) g_hInstance = GetModuleHandleA(NULL);

    if (!g_wndClass && !g_windowInited) {
        wcex.cbSize        = sizeof (WNDCLASSEX);
        wcex.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        //wcex.lpfnWndProc   = GameWndProc;
        wcex.cbClsExtra    = 0;
        wcex.cbWndExtra    = 0;
        wcex.hInstance     = g_hInstance;
        wcex.hIcon         = LoadIcon(g_hInstance, MAKEINTRESOURCE(109));
        wcex.hCursor       = LoadCursor(nullptr, IDC_ARROW);
        wcex.lpszMenuName  = nullptr;
        wcex.lpszClassName = "m_wndClass";
        wcex.hIconSm       = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(109));

        /* Nur für Schwarzhintergrund */
        wcex.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));

        Ensure(g_wndClass);
    }

    if (!g_hWnd && !g_windowInited) {
        UINT displayCount = 0;
        static RECT displayRects[9]{
            {0,0,0,0},
            {0,0,0,0},
            {0,0,0,0},
            {0,0,0,0},
            {0,0,0,0},
            {0,0,0,0},
            {0,0,0,0},
            {0,0,0,0},
            {0,0,0,0}
        };

        EnumDisplayMonitors(nullptr, nullptr,
            [](HMONITOR hMonitor, HDC, LPRECT pRect, LPARAM lParam) -> BOOL
            {
                if (!lParam || *(UINT *) lParam >= 9 || !pRect)
                    return FALSE;

                displayRects[*(UINT *) lParam] = *pRect;

                *(UINT *) lParam += 1;

                return TRUE;
            },
            (LPARAM) &displayCount);

        UINT displayIndex = config_get_d(CONFIG_DISPLAY) > displayCount - 1 ? displayCount - 1 : config_get_d(CONFIG_DISPLAY);

        RECT centerRect = { displayRects[displayIndex].left + ((displayRects[displayIndex].right  - displayRects[displayIndex].left) / 2),
                            displayRects[displayIndex].top  + ((displayRects[displayIndex].bottom - displayRects[displayIndex].top)  / 2),
                            0,
                            0 };

        UINT defaultWidth  = config_get_d(CONFIG_WIDTH);
        UINT defaultHeight = config_get_d(CONFIG_HEIGHT);

        bool bFullscreenDetected = (displayRects[displayIndex].right  - displayRects[displayIndex].left == defaultWidth &&
                                    displayRects[displayIndex].bottom - displayRects[displayIndex].top  == defaultHeight);

        RECT gameRect = { static_cast<long>(centerRect.left - (defaultWidth  / 2l)),
                          static_cast<long>(centerRect.top  - (defaultHeight / 2l)),
                          static_cast<long>(centerRect.left + (defaultWidth  / 2l)),
                          static_cast<long>(centerRect.top  + (defaultHeight / 2l)) };

        RECT gameRect { 0, 0, config_get_d(CONFIG_WIDTH), config_get_d(CONFIG_HEIGHT) };
        GetWindowRect(g_hWnd, &gameRect);

        POINT pos { gameRect.left, gameRect.top };

        HMONITOR    monitor = MonitorFromPoint(pos, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO monitorInfo{};
        monitorInfo.cbSize = sizeof(monitorInfo);
        GetMonitorInfo(monitor, &monitorInfo);

        if (f || bFullscreenDetected) {
            gameRect = { monitorInfo.rcMonitor.left,
                         monitorInfo.rcMonitor.top,
                         monitorInfo.rcMonitor.right  - monitorInfo.rcMonitor.left,
                         monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top, };
        }

        AdjustWindowRect(&gameRect,
                         f || bFullscreenDetected ? WS_POPUP | WS_VISIBLE : WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                         FALSE);

        Ensure((g_hWnd = CreateWindowEx(f || bFullscreenDetected ? WS_EX_APPWINDOW : WS_EX_WINDOWEDGE | WS_EX_APPWINDOW,
                                        wcex.lpszClassName,
                                        TITLE,
                                        f ? WS_POPUP | WS_VISIBLE : WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                        gameRect.left, gameRect.top,
                                        gameRect.right - gameRect.left, gameRect.bottom - gameRect.top,
                                        nullptr,
                                        nullptr,
                                        g_hInstance,
                                        nullptr)));

        if (f || bFullscreenDetected) video_directx12_fullscreen(true);
        g_windowInited = TRUE;
    }

    if (!g_dx12Inited) {
        UINT flags = 0;

#ifdef _DEBUG
        if SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&g_d3d12Debug))) {
            flags |= DXGI_CREATE_FACTORY_DEBUG;

            g_d3d12Debug->EnableDebugLayer();
            m_debugLayerEnabled = true;

            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&g_dxgiDebug))))
                g_dxgiDebug->EnableLeakTrackingForThread();
        }
#endif

        Ensure_HRESULT(CreateDXGIFactory2(flags, IID_PPV_ARGS(&g_dxgiFactory)));

        BOOL hwAdapterDone = FALSE;

        D3D_FEATURE_LEVEL minFeatureLevel = D3D_FEATURE_LEVEL_12_2;
        ComPtr<IDXGIAdapter1> out_hwAdapter;

        for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != g_dxgiFactory->EnumAdapters1(adapterIndex, &out_hwAdapter) && !hwAdapterDone; adapterIndex++) {
            if FAILED(out_hwAdapter.As(&g_dxgiHwAdapter)) continue;

            DXGI_ADAPTER_DESC1 desc1;
            g_dxgiHwAdapter->GetDesc1(&desc1);

            if (desc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;

            if (SUCCEEDED(D3D12CreateDevice(g_dxgiHwAdapter.Get(), minFeatureLevel, IID_PPV_ARGS(&g_d3d12Device)))) {
                hwAdapterDone = TRUE;
                break;
            }
        }

        DXGI_QUERY_VIDEO_MEMORY_INFO videoInfo;

        if (hwAdapterDone) {
//#ifdef NDEBUG
            Ensure_HRESULT(g_dxgiHwAdapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &videoInfo));

            if (videoInfo.AvailableForReservation / 1024 / 1024 < 16) {
                MessageBoxA(nullptr,
                    "Not enough VRAM available for this application.\n"
                    "Please close other applications and try again.\n\n"
                    "If the problem persists, try using newer graphics card.", "Error", MB_OK | MB_ICONERROR);
                return false;
            }
//#endif
        } else {
            Ensure_HRESULT(g_dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&g_dxgiWarpAdapter)));
            Ensure_HRESULT(D3D12CreateDevice(g_dxgiWarpAdapter.Get(), minFeatureLevel, IID_PPV_ARGS(&g_d3d12Device)));

#ifdef NDEBUG
            Ensure_HRESULT(g_dxgiWarpAdapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &videoInfo));

            if (videoInfo.AvailableForReservation / 1024 / 1024 < 16) {
                MessageBoxA(nullptr,
                    "Not enough VRAM available for this application.\n"
                    "Please close other applications and try again.\n\n"
                    "If the problem persists, try using newer graphics card.", "Error", MB_OK | MB_ICONERROR);
                return false;
            }
#endif
        }

        // === Funktionsunterstützung ===

        g_max_samples = 16;
        const int samples = config_get_d(CONFIG_MULTISAMPLE);

        if (g_max_samples > 1) {
            for (; g_max_samples > 1; g_max_samples--) {
                if (g_max_samples == 1)
                    break;

                D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS levels = { DXGI_FORMAT_B8G8R8A8_UNORM, g_max_samples, D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE, 0u };
                if (FAILED(g_d3d12Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &levels, sizeof(levels))))
                    continue;

                if (levels.NumQualityLevels > 0)
                    break;
            }
        }

        if (samples > g_max_samples)
            config_set_d(CONFIG_MULTISAMPLE, g_max_samples);

        // === Ende Funktionsunterstützung ===

        D3D12_COMMAND_QUEUE_DESC cmdQDesc = {};
        cmdQDesc.Type     = D3D12_COMMAND_LIST_TYPE_DIRECT;
        cmdQDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        cmdQDesc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
        cmdQDesc.NodeMask = 0;

        Ensure_HRESULT(g_d3d12Device->CreateCommandQueue(&cmdQDesc, IID_PPV_ARGS(&g_cmdQueue)));

        RECT clientRect;
        GetClientRect(g_hWnd, &clientRect);

        ComPtr<IDXGISwapChain1> swapChain;
        DXGI_SWAP_CHAIN_DESC1           scd1 = {0};
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC sfd  = {0};

        scd1.Width       = static_cast<UINT>(clientRect.right);
        scd1.Height      = static_cast<UINT>(clientRect.bottom);
        scd1.Format      = DXGI_FORMAT_B8G8R8A8_UNORM;
        scd1.Stereo      = FALSE;
        scd1.SampleDesc  = { 1, 0 };
        scd1.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scd1.BufferCount = VIDEO_D3D12_MAX_FRAME_COUNT;
        scd1.Scaling     = DXGI_SCALING_STRETCH;
        scd1.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        scd1.AlphaMode   = DXGI_ALPHA_MODE_IGNORE;
        scd1.Flags       = 0; //DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        sfd .Windowed    = TRUE;

        Ensure_HRESULT(g_dxgiFactory->CreateSwapChainForHwnd(
            g_cmdQueue.Get(),
            g_hWnd,
            &scd1, &sfd,
            nullptr,
            &swapChain));

        Ensure_HRESULT(swapChain.As(&g_swapChain));

        /* Nur F11 Taste nutzen, um Performance verbessern zu können. */
        g_dxgiFactory->MakeWindowAssociation(g_hWnd, DXGI_MWA_NO_ALT_ENTER);

        g_currentFrame = g_swapChain->GetCurrentBackBufferIndex();

        {
            CD3DX12_DESCRIPTOR_RANGE range_SRV;
            CD3DX12_DESCRIPTOR_RANGE range_SRV_ShadowMap;
            CD3DX12_ROOT_PARAMETER   params[3];

            range_SRV.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
            range_SRV_ShadowMap.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5);
            params[0].InitAsConstantBufferView(0);
            params[1].InitAsDescriptorTable(1, &range_SRV,           D3D12_SHADER_VISIBILITY_PIXEL);
            params[2].InitAsDescriptorTable(1, &range_SRV_ShadowMap, D3D12_SHADER_VISIBILITY_PIXEL);

            D3D12_ROOT_SIGNATURE_FLAGS rsFlags =
                D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS   |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS     |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;

            D3D12_STATIC_SAMPLER_DESC ssDescs[5]{};
            ssDescs[0] = InitStaticSamplerDesc(0, D3D12_FILTER_ANISOTROPIC);
            ssDescs[1] = InitStaticSamplerDesc(1, D3D12_FILTER_ANISOTROPIC, true, false);
            ssDescs[2] = InitStaticSamplerDesc(2, D3D12_FILTER_ANISOTROPIC, false, true);
            ssDescs[3] = InitStaticSamplerDesc(3, D3D12_FILTER_ANISOTROPIC, true, true);

            CD3DX12_ROOT_SIGNATURE_DESC rsDesc;
            rsDesc.Init(_countof(params), params, _countof(ssDescs), ssDescs, rsFlags);

            ComPtr<ID3DBlob> pRS, pError;
            Ensure_HRESULT(D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pRS, &pError));
            Ensure_HRESULT(g_d3d12Device->CreateRootSignature(0, pRS->GetBufferPointer(), pRS->GetBufferSize(), IID_PPV_ARGS(&g_rootSignature)));
        }

        {
            D3D12_INPUT_ELEMENT_DESC defaultIADescs[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            };

            D3D12_INPUT_ELEMENT_DESC interfaceIADescs[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 8,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            };

            // Default PSO

            WIN_ShaderFile vsDefault("cso/VS_Default.cso");
            WIN_ShaderFile psLit    ("cso/PS_Lit.cso");

            auto psoDefaultDesc = InitPSODesc(defaultIADescs, _countof(defaultIADescs),
                g_rootSignature.Get(),
                { vsDefault.GetBuffer(), vsDefault.GetSize() },
                { psLit.GetBuffer(),     psLit.GetSize() },
                DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_D24_UNORM_S8_UINT, false, false, false, config_get_d(CONFIG_MULTISAMPLE));

            Ensure_HRESULT(g_d3d12Device->CreateGraphicsPipelineState(&psoDefaultDesc, IID_PPV_ARGS(&g_psoDefault)));

            // Double-Sided PSO

            auto psoDoubleSidedDesc = InitPSODesc(defaultIADescs, _countof(defaultIADescs),
                g_rootSignature.Get(),
                { vsDefault.GetBuffer(), vsDefault.GetSize() },
                { psLit.GetBuffer(),     psLit.GetSize() },
                DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_D24_UNORM_S8_UINT, false, true, false, config_get_d(CONFIG_MULTISAMPLE));

            Ensure_HRESULT(g_d3d12Device->CreateGraphicsPipelineState(&psoDoubleSidedDesc, IID_PPV_ARGS(&g_psoDoubleSided)));

            // User Interface PSO

            WIN_ShaderFile vsUI("cso/VS_UI.cso");
            WIN_ShaderFile psUI("cso/PS_UI.cso");

            auto psoUIDesc = InitPSODesc(interfaceIADescs, _countof(interfaceIADescs),
                g_rootSignature.Get(),
                { vsUI.GetBuffer(), vsUI.GetSize() },
                { psUI.GetBuffer(), psUI.GetSize() },
                DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_D24_UNORM_S8_UINT, false, false, true, config_get_d(CONFIG_MULTISAMPLE));

            Ensure_HRESULT(g_d3d12Device->CreateGraphicsPipelineState(&psoUIDesc, IID_PPV_ARGS(&g_psoUserInterface)));
        }

        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc {};
        rtvHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.NumDescriptors = VIDEO_D3D12_MAX_FRAME_COUNT;
        rtvHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        Ensure_HRESULT(g_d3d12Device->CreateDescriptorHeap(
            &rtvHeapDesc,
            IID_PPV_ARGS(&g_rtvHeap)));

        auto rtvFirstHandle     = g_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        auto rtvHandleIncrement = g_d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        for (size_t i = 0; i < VIDEO_D3D12_MAX_FRAME_COUNT; ++i) {
            g_rtvHandles[i]      = rtvFirstHandle;
            g_rtvHandles[i].ptr += rtvHandleIncrement * i;
        }

        g_rtvDescriptorSize = rtvHandleIncrement;

        // === MSAA RT Heap Init ===

        D3D12_DESCRIPTOR_HEAP_DESC msaaRtvHeapDesc {};
        msaaRtvHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        msaaRtvHeapDesc.NumDescriptors = VIDEO_D3D12_MAX_FRAME_COUNT;
        msaaRtvHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        Ensure_HRESULT(g_d3d12Device->CreateDescriptorHeap(
            &msaaRtvHeapDesc,
            IID_PPV_ARGS(&g_msaaRtvHeap)));

        // === End MSAA RT Heap Init ===

        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc {};
        dsvHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        Ensure_HRESULT(g_d3d12Device->CreateDescriptorHeap(
            &dsvHeapDesc,
            IID_PPV_ARGS(&g_dsvHeap)));

        // === MSAA DS Heap Init ===

        D3D12_DESCRIPTOR_HEAP_DESC msaaDsvHeapDesc {};
        msaaDsvHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        msaaDsvHeapDesc.NumDescriptors = 1;
        msaaDsvHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        Ensure_HRESULT(g_d3d12Device->CreateDescriptorHeap(
            &msaaDsvHeapDesc,
            IID_PPV_ARGS(&g_msaaDsvHeap)));

        // === End MSAA DS Heap Init ===

        // === CBV Init ===

        D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC cbvDesc = CD3DX12_RESOURCE_DESC::Buffer(VIDEO_D3D12_MAX_FRAME_COUNT * c_aligned_size_CB_ModelObject);

        for (UINT n = 0; n < VIDEO_D3D12_MAX_FRAME_COUNT; n++) {
            g_cbvModelObjectStates[n] = D3D12_RESOURCE_STATE_GENERIC_READ;
            Ensure_HRESULT(
                g_d3d12Device->CreateCommittedResource(
                    &uploadHeapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &cbvDesc,
                    g_cbvModelObjectStates[n],
                    nullptr,
                    IID_PPV_ARGS(&g_cbvModelObject[n])
                )
            );

            CD3DX12_RANGE readRange(0, 0);
            Ensure_HRESULT(g_cbvModelObject[n]->Map(0, &readRange, reinterpret_cast<void**>(&g_mappedCbModelObject[n])));

            UINT8* dst = g_mappedCbModelObject[n] + (c_aligned_size_CB_ModelObject * n);
            CopyMemory(dst, &g_cbModelObjectData, sizeof(g_cbModelObjectData));
        }

        // === End CBV Init ===

        // === CMDLIST Init ===

        for (UINT i = 0; i < VIDEO_D3D12_MAX_FRAME_COUNT; i++) {
            Ensure_HRESULT(g_d3d12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_cmdAllocators[i])));
        }

        Ensure_HRESULT(g_d3d12Device->CreateCommandList(
                       0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                       g_cmdAllocators[g_currentFrame].Get(),
                       g_psoDefault.Get(),
                       IID_PPV_ARGS(&g_cmdList)));

        Ensure_HRESULT(g_d3d12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence)));

        g_fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (g_fenceEvent == nullptr)
            Ensure_HRESULT(HRESULT_FROM_WIN32(GetLastError()));

        // === End CMDLIST Init ===

        g_dx12Inited = true;
    }

    Ensure(UpdateRenderViews());

    return g_dx12Inited;
}

static void video_directx12_shutdown(void)
{
    if (g_dx12Inited) {
        Flush();

        g_dx12Inited = false;

#ifdef _DEBUG
        if (g_dxgiDebug) {
            OutputDebugString("(i) AirlineAircraft DebugLayer: DXGI Reports living device objects:\n");
            g_dxgiDebug->ReportLiveObjects(
                DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL)
            );
        }
#endif

        if (g_fenceEvent) CloseHandle(g_fenceEvent);
        if (g_fence)      g_fence.Reset();
        if (g_cmdList)    g_cmdList.Reset();

        for (UINT n = 0; n < VIDEO_D3D12_MAX_FRAME_COUNT; n++) {
            if (g_cbvModelObject[n] != nullptr)
            {
                g_cbvModelObject[n]->Unmap(0, nullptr);
                g_cbvModelObject[n].Reset();
            }
        }

        if (g_msaaDsvHeap) g_msaaDsvHeap.Reset();
        if (g_dsvHeap)     g_dsvHeap.Reset();
        if (g_msaaRtvHeap) g_msaaRtvHeap.Reset();
        if (g_rtvHeap)     g_rtvHeap.Reset();

        if (g_psoUserInterface) g_psoUserInterface.Reset();
        if (g_psoReflective)    g_psoReflective.Reset();
        if (g_psoDoubleSided)   g_psoDoubleSided.Reset();
        if (g_psoDefault)       g_psoDefault.Reset();
        if (g_rootSignature)    g_rootSignature.Reset();
        if (g_swapChain)        g_swapChain.Reset();
        if (g_cmdQueue)         g_cmdQueue.Reset();
        if (g_infoQueue)        g_infoQueue.Reset();
        if (g_d3d12Device)      g_d3d12Device.Reset();
        if (g_dxgiWarpAdapter)  g_dxgiWarpAdapter.Reset();
        if (g_dxgiHwAdapter)    g_dxgiHwAdapter.Reset();
        if (g_dxgiFactory)      g_dxgiFactory.Reset();

#ifdef _DEBUG
        if (g_dxgiDebug)  g_dxgiDebug.Reset();
        if (g_d3d12Debug) g_d3d12Debug.Reset();
#endif
    }
}

extern "C" void video_directx12_quit(void)
{
    video_directx12_shutdown();

    if (g_hWnd) {
        DestroyWindow(g_hWnd);
        g_hWnd = 0;
    }

    if (g_wndClass) {
        UnregisterClassA("g_wndClass", g_hInstance);
        g_wndClass = 0;
    }
}

/********************************************************************/

extern "C" void video_directx12_set_perspective(float fov, float n, float f)
{
    g_mat_projection = DirectX::XMMatrixPerspectiveFovRH(
        fov,
        static_cast<FLOAT>(video.device_w) /
        static_cast<FLOAT>(video.device_h),
        n, f
    );

    g_mat_modelview[g_mat_modelview_depthRemaining] = DirectX::XMMatrixIdentity();
}

extern "C" void video_directx12_set_ortho(void)
{
    g_mat_projection = DirectX::XMMatrixOrthographicRH(
        video.device_w, video.device_h,
        -1.0f, 1.0f);

    g_mat_modelview[g_mat_modelview_depthRemaining] = DirectX::XMMatrixIdentity();
}

/*
 * Prepare DX12's Texture Shader Resource View.
 * Use this function before update the mapped CBV.
 */
extern "C" void video_directx12_prepare_srv(const int mode)
{
    //g_cmdList->SetGraphicsRootSignature(g_rootSignature.Get());

    g_cbModelObjectData.g_texture_mode = mode;
}

/*
 * Prepare DX12's Constant Buffer View.
 * Do not update texture mode after this function.
 */
extern "C" void video_directx12_prepare_cbv(const int ui, const int lit, const int gui)
{
    DirectX::XMMATRIX mat_modelview = DirectX::XMMatrixIdentity();

    for (UINT i = g_mat_modelview_depthRemaining; i < 32; i++)
        mat_modelview *= g_mat_modelview[i];

    DirectX::XMStoreFloat4x4(&g_cbModelObjectData.g_wvp_mtx, mat_modelview * g_mat_projection);

    if (g_cbModelObjectData.g_lit_mode != 1 && lit && !gui)
        g_cbModelObjectData.g_lit_mode = 1;
    else if (g_cbModelObjectData.g_lit_mode != 0)
        g_cbModelObjectData.g_lit_mode = 0;

    //g_cmdList->SetGraphicsRootSignature(g_rootSignature.Get());

    UINT8* dst = g_mappedCbModelObject[g_currentFrame] + (c_aligned_size_CB_ModelObject * g_currentCbvIndex);
    CopyMemory(dst, &g_cbModelObjectData, sizeof (g_cbModelObjectData));

    g_cmdList->SetGraphicsRootConstantBufferView(0, g_cbvModelObject[g_currentFrame]->GetGPUVirtualAddress() + (c_aligned_size_CB_ModelObject + g_currentCbvIndex));

    g_currentCbvIndex++;
}

extern "C" BOOL video_directx12_swap(void)
{
    if (!g_framePrepared) return TRUE;

    g_framePrepared = FALSE;

    if (config_get_d(CONFIG_MULTISAMPLE) > 1) {
        NextResourceState(g_msaaRenderTarget.Get(), &g_msaaRenderTargetState, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);

        g_cmdList->ResolveSubresource(g_renderTargets[g_currentFrame].Get(), 0, g_msaaRenderTarget.Get(), 0, DXGI_FORMAT_B8G8R8A8_UNORM);
    }

    NextResourceState(g_renderTargets[g_currentFrame].Get(), &g_renderTargetStates[g_currentFrame], D3D12_RESOURCE_STATE_PRESENT);

    Ensure_HRESULT(g_cmdList->Close());
    ID3D12CommandList *const cmdLists[] = { g_cmdList.Get() };
    g_cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
    Ensure(WaitForGPU());

    HRESULT hr = g_swapChain->Present(config_get_d(CONFIG_VSYNC) == 1 ? 1 : 0, 0);
#ifndef NDEBUG
    if (hr == DXGI_ERROR_DEVICE_REMOVED ||
        hr == DXGI_ERROR_DEVICE_RESET) {
        video_directx12_shutdown();
        return video_directx12_init(config_get_d(CONFIG_FULLSCREEN),
                                    config_get_d(CONFIG_WIDTH),
                                    config_get_d(CONFIG_HEIGHT));
    }
    else
#endif
        Ensure_HRESULT(hr);

    Ensure(NextFrame());

    return TRUE;
}

extern "C" BOOL video_directx12_prepare_frame(void)
{
    if (g_framePrepared) return TRUE;

    Ensure_HRESULT(g_cmdAllocators[g_currentFrame]->Reset());
    Ensure_HRESULT(g_cmdList->Reset(g_cmdAllocators[g_currentFrame].Get(), g_psoDefault.Get()));

    g_framePrepared = TRUE;

    return TRUE;
}

extern "C" BOOL video_directx12_clear(void)
{
    if (!g_framePrepared) return TRUE;

    RECT gameRect;
    GetClientRect(g_hWnd, &gameRect);

    D3D12_VIEWPORT screenViewport = { 0, 0, static_cast<float>(gameRect.right), static_cast<float>(gameRect.bottom), 0.f, 1.f };
    D3D12_RECT     scissorRect    = { 0, 0, gameRect.right, gameRect.bottom };

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = config_get_d(CONFIG_MULTISAMPLE) > 1 ? g_msaaRtvHeap->GetCPUDescriptorHandleForHeapStart() : g_rtvHandles[g_currentFrame];
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = config_get_d(CONFIG_MULTISAMPLE) > 1 ? g_msaaDsvHeap->GetCPUDescriptorHandleForHeapStart() : g_dsvHeap->GetCPUDescriptorHandleForHeapStart();

    const float c_clearColor[4] = { 0, 0, 0, 0 };

    if (config_get_d(CONFIG_MULTISAMPLE) > 1) {
        NextResourceState(g_msaaRenderTarget.Get(), &g_msaaRenderTargetState, D3D12_RESOURCE_STATE_RENDER_TARGET);
        NextResourceState(g_renderTargets[g_currentFrame].Get(), &g_renderTargetStates[g_currentFrame], D3D12_RESOURCE_STATE_RESOLVE_DEST);
    }
    else NextResourceState(g_renderTargets[g_currentFrame].Get(), &g_renderTargetStates[g_currentFrame], D3D12_RESOURCE_STATE_RENDER_TARGET);

    g_cmdList->RSSetViewports(1, &screenViewport);
    g_cmdList->RSSetScissorRects(1, &scissorRect);

    g_cmdList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
    g_cmdList->ClearRenderTargetView(rtv, c_clearColor, 0, nullptr);
    g_cmdList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    g_currentCbvIndex = 0;
    g_cmdList->SetGraphicsRootSignature(g_rootSignature.Get());

    return true;
}

/*---------------------------------------------------------------------------*/

extern "C" HRESULT directx12_next_resource_state(ID3D12Resource2       *ptr_resource,
                                                 D3D12_RESOURCE_STATES *curr_state,
                                                 D3D12_RESOURCE_STATES  next_state)
{
    Ensure_HRESULT(NextResourceState(ptr_resource, curr_state, next_state));
    return S_OK;
}

extern "C" HRESULT directx12_create_buffer(UINT size, D3D12_RESOURCE_BUFFER_OBJECT *obj)
{
    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(size);

    for (UINT i = 0; i < size; i++)
        Ensure_HRESULT(
            g_d3d12Device->CreateCommittedResource(
                &uploadHeap,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&obj[i].m_objectResource)
            ));

    return S_OK;
}

extern "C" void directx12_delete_buffer(UINT size, D3D12_RESOURCE_BUFFER_OBJECT *obj)
{
    for (UINT i = 0; i < size; i++)
        if (obj[i].m_objectResource != nullptr) obj[i].m_objectResource.Reset();
}

extern "C" HRESULT directx12_set_buffer_data(D3D12_RESOURCE_BUFFER_OBJECT obj, long size, PVOID data)
{
    Ensure_HRESULT(NextResourceState(obj.m_objectResource.Get(), &obj.m_objectResourceState, D3D12_RESOURCE_STATE_GENERIC_READ));

    void *mappedData = nullptr;
    D3D12_RANGE readRange = { 0, 0 }; // We do not intend to read this resource on CPU
    Ensure_HRESULT(obj.m_objectResource->Map(0, &readRange, &mappedData));
    memcpy((char *) mappedData, data, size);
    obj.m_objectResource->Unmap(0, nullptr);

    return S_OK;
}

extern "C" HRESULT directx12_set_buffer_subdata(D3D12_RESOURCE_BUFFER_OBJECT obj, long offset, long size, PVOID data)
{
    Ensure_HRESULT(NextResourceState(obj.m_objectResource.Get(), &obj.m_objectResourceState, D3D12_RESOURCE_STATE_GENERIC_READ));

    void *mappedData = nullptr;
    D3D12_RANGE readRange = { 0, 0 }; // We do not intend to read this resource on CPU
    Ensure_HRESULT(obj.m_objectResource->Map(0, &readRange, &mappedData));
    memcpy((char *) mappedData + offset, data, size);
    obj.m_objectResource->Unmap(0, nullptr);

    return S_OK;
}

extern "C" void directx12_push_matrix_modelview(void)
{
    if (g_mat_modelview_depthRemaining <= 0) return;

    g_mat_modelview_depthRemaining--;
    g_mat_modelview[g_mat_modelview_depthRemaining] = DirectX::XMMatrixIdentity();
}

extern "C" void directx12_pop_matrix_modelview(void)
{
    if (g_mat_modelview_depthRemaining >= 31) return;

    g_mat_modelview[g_mat_modelview_depthRemaining] = DirectX::XMMatrixIdentity();
    g_mat_modelview_depthRemaining++;
}

extern "C" void directx12_matrix_translatef(float x, float y, float z)
{
    g_mat_modelview[g_mat_modelview_depthRemaining] =
        DirectX::XMMatrixTranslation(x, y, z) *
        g_mat_modelview[g_mat_modelview_depthRemaining];
}

extern "C" void directx12_matrix_translatef(float pos[3])
{
    g_mat_modelview[g_mat_modelview_depthRemaining] =
        DirectX::XMMatrixTranslation(pos[0], pos[1], pos[2]) *
        g_mat_modelview[g_mat_modelview_depthRemaining];
}

extern "C" void directx12_matrix_scalef(float x, float y, float z)
{
    g_mat_modelview[g_mat_modelview_depthRemaining] =
        DirectX::XMMatrixScaling(x, y, z) *
        g_mat_modelview[g_mat_modelview_depthRemaining];
}

extern "C" void directx12_matrix_scalef(float pos[3])
{
    g_mat_modelview[g_mat_modelview_depthRemaining] =
        DirectX::XMMatrixScaling(pos[0], pos[1], pos[2]) *
        g_mat_modelview[g_mat_modelview_depthRemaining];
}

extern "C" void directx12_matrix_rotatef(float angle, float x, float y, float z)
{
    DirectX::FXMVECTOR axis = { x, y, z, 1.0f };

    g_mat_modelview[g_mat_modelview_depthRemaining] =
        DirectX::XMMatrixRotationAxis(axis, angle) *
        g_mat_modelview[g_mat_modelview_depthRemaining];
}

extern "C" void directx12_matrix_rotatef(float angle, float vaxis[3])
{
    DirectX::FXMVECTOR axis = { vaxis[0], vaxis[1], vaxis[2], 1.0f };

    g_mat_modelview[g_mat_modelview_depthRemaining] =
        DirectX::XMMatrixRotationAxis(axis, angle) *
        g_mat_modelview[g_mat_modelview_depthRemaining];
}

extern "C" void directx12_matrix_set(float m[16])
{
    DirectX::XMFLOAT4X4 tab = DirectX::XMFLOAT4X4(m);

    g_mat_modelview[g_mat_modelview_depthRemaining] =
        DirectX::XMLoadFloat4x4(&tab);
}

extern "C" void directx12_matrix_multiply(float m[16])
{
    DirectX::XMFLOAT4X4 tab = DirectX::XMFLOAT4X4(m);

    g_mat_modelview[g_mat_modelview_depthRemaining] =
        DirectX::XMLoadFloat4x4(&tab) *
        g_mat_modelview[g_mat_modelview_depthRemaining];
}

extern "C" void directx12_color4ub(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    g_cbModelObjectData.g_color4 = DirectX::XMFLOAT4(
        r / 255.0f,
        g / 255.0f,
        b / 255.0f,
        a / 255.0f
    );
}

extern "C" void directx12_color4ub(unsigned char vcolor[4])
{
    g_cbModelObjectData.g_color4 = DirectX::XMFLOAT4(
        vcolor[0] / 255.0f,
        vcolor[1] / 255.0f,
        vcolor[2] / 255.0f,
        vcolor[3] / 255.0f
    );
}

extern "C" void directx12_color4f(float r, float g, float b, float a)
{
    g_cbModelObjectData.g_color4 = DirectX::XMFLOAT4(r, g, b, a);
}

extern "C" void directx12_color4f(float vcolor[4])
{
    g_cbModelObjectData.g_color4 = DirectX::XMFLOAT4(vcolor);
}

extern "C" void directx12_set_viewport(int x, int y, int w, int h)
{
    D3D12_VIEWPORT screenViewport = { x, y, w, h, 0.f, 1.f };
    g_cmdList->RSSetViewports(1, &screenViewport);
}

extern "C" void directx12_set_scissor(int x, int y, int w, int h)
{
    D3D12_RECT scissorRect = { x, y, w, h };
    g_cmdList->RSSetScissorRects(1, &scissorRect);
}

extern "C" void directx12_reset_scissor(void)
{
    RECT gameRect;
    GetClientRect(g_hWnd, &gameRect);

    D3D12_RECT scissorRect = { 0, 0, gameRect.right, gameRect.bottom };
    g_cmdList->RSSetScissorRects(1, &scissorRect);
}

/*---------------------------------------------------------------------------*/

#endif
