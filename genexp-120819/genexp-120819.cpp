#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <dxgi1_4.h>
#define WIN32_LEAN_AND_MEAN
#include <d3d12.h>
#ifndef __INTELLISENSE__
#include "genexp-120819.hlsl.h"
#endif

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#define VHR(hr) if (FAILED(hr)) { assert(0); }
#define SAFE_RELEASE(obj) if ((obj)) { (obj)->Release(); (obj) = nullptr; }

#define GResolutionX 1280
#define GResolutionY 720
#define GNumSwapBuffers 4
static ID3D12Device2* GDevice;
static ID3D12CommandQueue* GCmdQueue;
static ID3D12CommandAllocator* GCmdAlloc[2];
static ID3D12GraphicsCommandList2* GCmdList;
static IDXGISwapChain3* GSwapChain;
static ID3D12DescriptorHeap* GRTVHeap;
static ID3D12Resource* GSwapBuffers[GNumSwapBuffers];
static ID3D12Fence* GFrameFence;
static HANDLE GFrameFenceEvent;
static uint64_t GNumFrames;
static uint32_t GFrameIndex;
static ID3D12DescriptorHeap* GDescriptorHeap;
static ID3D12Resource* GComputeTexture;
static D3D12_GPU_DESCRIPTOR_HANDLE GComputeTextureUAV;
static ID3D12PipelineState* GPipelineState;
static ID3D12RootSignature* GRootSignature;

static void Draw(float Time)
{
    GCmdAlloc[GFrameIndex]->Reset();
    GCmdList->Reset(GCmdAlloc[GFrameIndex], nullptr);
    GCmdList->SetDescriptorHeaps(1, &GDescriptorHeap);

    // Generate image in a compute shader.
    GCmdList->SetPipelineState(GPipelineState);
    GCmdList->SetComputeRootSignature(GRootSignature);
    GCmdList->SetComputeRootDescriptorTable(0, GComputeTextureUAV);
    GCmdList->SetComputeRoot32BitConstants(1, 1, &Time, 0);
    GCmdList->Dispatch(GResolutionX / 8, GResolutionY / 8, 1);

    ID3D12Resource* BackBuffer = GSwapBuffers[GSwapChain->GetCurrentBackBufferIndex()];

    {
        D3D12_RESOURCE_BARRIER Barriers[2] = {};

        Barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        Barriers[0].Transition.pResource = BackBuffer;
        Barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        Barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;

        Barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        Barriers[1].Transition.pResource = GComputeTexture;
        Barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        Barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

        GCmdList->ResourceBarrier(2, Barriers);
    }

    GCmdList->CopyResource(BackBuffer, GComputeTexture);

    {
        D3D12_RESOURCE_BARRIER Barriers[2] = {};

        Barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        Barriers[0].Transition.pResource = BackBuffer;
        Barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        Barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

        Barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        Barriers[1].Transition.pResource = GComputeTexture;
        Barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
        Barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

        GCmdList->ResourceBarrier(2, Barriers);
    }
    GCmdList->Close();

    GCmdQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList**>(&GCmdList));
}

static void CreateGraphicsContext(HWND Window)
{
    IDXGIFactory4* Factory;
#ifdef _DEBUG
    VHR(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&Factory)));
#else
    VHR(CreateDXGIFactory2(0, IID_PPV_ARGS(&Factory)));
#endif
#ifdef _DEBUG
    {
        ID3D12Debug* Dbg;
        D3D12GetDebugInterface(IID_PPV_ARGS(&Dbg));
        if (Dbg)
        {
            Dbg->EnableDebugLayer();
            ID3D12Debug1* Dbg1;
            Dbg->QueryInterface(IID_PPV_ARGS(&Dbg1));
            if (Dbg1)
            {
                Dbg1->SetEnableGPUBasedValidation(TRUE);
            }
            SAFE_RELEASE(Dbg);
            SAFE_RELEASE(Dbg1);
        }
    }
#endif
    if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&GDevice))))
    {
        MessageBox(Window, "This application requires Windows 10 1709 (RS3) or newer.", "D3D12CreateDevice failed", MB_OK | MB_ICONERROR);
        return;
    }

    D3D12_COMMAND_QUEUE_DESC CmdQueueDesc = {};
    CmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    CmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    CmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    VHR(GDevice->CreateCommandQueue(&CmdQueueDesc, IID_PPV_ARGS(&GCmdQueue)));

    DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};
    SwapChainDesc.BufferCount = GNumSwapBuffers;
    SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDesc.OutputWindow = Window;
    SwapChainDesc.SampleDesc.Count = 1;
    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    SwapChainDesc.Windowed = TRUE;

    IDXGISwapChain* TempSwapChain;
    VHR(Factory->CreateSwapChain(GCmdQueue, &SwapChainDesc, &TempSwapChain));
    VHR(TempSwapChain->QueryInterface(IID_PPV_ARGS(&GSwapChain)));
    SAFE_RELEASE(TempSwapChain);
    SAFE_RELEASE(Factory);

    for (uint32_t Idx = 0; Idx < 2; ++Idx)
    {
        VHR(GDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&GCmdAlloc[Idx])));
    }

    // Render target descriptor heap (RTV).
    {
        D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
        HeapDesc.NumDescriptors = GNumSwapBuffers;
        HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        VHR(GDevice->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&GRTVHeap)));
    }

    // Swap-buffer render targets.
    {
        const uint32_t DescriptorSizeRTV = GDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_CPU_DESCRIPTOR_HANDLE Handle = GRTVHeap->GetCPUDescriptorHandleForHeapStart();

        for (uint32_t Idx = 0; Idx < GNumSwapBuffers; ++Idx)
        {
            VHR(GSwapChain->GetBuffer(Idx, IID_PPV_ARGS(&GSwapBuffers[Idx])));
            GDevice->CreateRenderTargetView(GSwapBuffers[Idx], nullptr, Handle);
            Handle.ptr += DescriptorSizeRTV;
        }
    }

    VHR(GDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, GCmdAlloc[0], nullptr, IID_PPV_ARGS(&GCmdList)));
    VHR(GCmdList->Close());

    VHR(GDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&GFrameFence)));
    GFrameFenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);

    {
        D3D12_RESOURCE_DESC TextureDesc = {};
        TextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        TextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        TextureDesc.Width = GResolutionX;
        TextureDesc.Height = GResolutionY;
        TextureDesc.DepthOrArraySize = 1;
        TextureDesc.MipLevels = 1;
        TextureDesc.SampleDesc.Count = 1;
        TextureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        D3D12_HEAP_PROPERTIES TextureHeapDesc = {};
        TextureHeapDesc.Type = D3D12_HEAP_TYPE_DEFAULT;

        VHR(GDevice->CreateCommittedResource(&TextureHeapDesc, D3D12_HEAP_FLAG_NONE, &TextureDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&GComputeTexture)));

        D3D12_DESCRIPTOR_HEAP_DESC DescriptorHeapDesc = {};
        DescriptorHeapDesc.NumDescriptors = 1;
        DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        VHR(GDevice->CreateDescriptorHeap(&DescriptorHeapDesc, IID_PPV_ARGS(&GDescriptorHeap)));

        D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle = GDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        GComputeTextureUAV = GDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

        GDevice->CreateUnorderedAccessView(GComputeTexture, nullptr, nullptr, CPUHandle);
    }

    {
#ifndef __INTELLISENSE__
        D3D12_COMPUTE_PIPELINE_STATE_DESC PSODesc = {};
        PSODesc.CS = { g_MainCS, sizeof(g_MainCS) };
        VHR(GDevice->CreateComputePipelineState(&PSODesc, IID_PPV_ARGS(&GPipelineState)));
        VHR(GDevice->CreateRootSignature(0, g_MainCS, sizeof(g_MainCS), IID_PPV_ARGS(&GRootSignature)));
#endif
    }
}

static void DestroyGraphicsContext()
{
    SAFE_RELEASE(GRootSignature);
    SAFE_RELEASE(GPipelineState);
    SAFE_RELEASE(GComputeTexture);
    SAFE_RELEASE(GDescriptorHeap);
    SAFE_RELEASE(GFrameFence);
    CloseHandle(GFrameFenceEvent);
    for (uint32_t Idx = 0; Idx < GNumSwapBuffers; ++Idx)
    {
        SAFE_RELEASE(GSwapBuffers[Idx]);
    }
    SAFE_RELEASE(GRTVHeap);
    SAFE_RELEASE(GSwapChain);
    SAFE_RELEASE(GCmdList);
    SAFE_RELEASE(GCmdAlloc[0]);
    SAFE_RELEASE(GCmdAlloc[1]);
    SAFE_RELEASE(GCmdQueue);
    SAFE_RELEASE(GDevice);
}

static void PresentFrame()
{
    GSwapChain->Present(0, 0);
    GCmdQueue->Signal(GFrameFence, ++GNumFrames);

    const uint64_t NumGPUFrames = GFrameFence->GetCompletedValue();

    if ((GNumFrames - NumGPUFrames) >= 2)
    {
        GFrameFence->SetEventOnCompletion(NumGPUFrames + 1, GFrameFenceEvent);
        WaitForSingleObject(GFrameFenceEvent, INFINITE);
    }

    GFrameIndex = !GFrameIndex;
}

static void WaitForGPU()
{
    GCmdQueue->Signal(GFrameFence, ++GNumFrames);
    GFrameFence->SetEventOnCompletion(GNumFrames, GFrameFenceEvent);
    WaitForSingleObject(GFrameFenceEvent, INFINITE);
}

static double GetTime()
{
    static LARGE_INTEGER StartCounter;
    static LARGE_INTEGER Frequency;
    if (StartCounter.QuadPart == 0)
    {
        QueryPerformanceFrequency(&Frequency);
        QueryPerformanceCounter(&StartCounter);
    }
    LARGE_INTEGER Counter;
    QueryPerformanceCounter(&Counter);
    return (Counter.QuadPart - StartCounter.QuadPart) / static_cast<double>(Frequency.QuadPart);
}

static void UpdateFrameStats(HWND Window, const char* Name, double& OutTime, float& OutDeltaTime)
{
    static double PreviousTime = -1.0;
    static double HeaderRefreshTime = 0.0;
    static uint32_t NumFrames = 0;

    if (PreviousTime < 0.0)
    {
        PreviousTime = GetTime();
        HeaderRefreshTime = PreviousTime;
    }

    OutTime = GetTime();
    OutDeltaTime = static_cast<float>(OutTime - PreviousTime);
    PreviousTime = OutTime;

    if ((OutTime - HeaderRefreshTime) >= 1.0)
    {
        const double FPS = NumFrames / (OutTime - HeaderRefreshTime);
        const double MS = (1.0 / FPS) * 1000.0;
        char Header[256];
        snprintf(Header, sizeof(Header), "[%.1f fps  %.3f ms] %s", FPS, MS, Name);
        SetWindowText(Window, Header);
        HeaderRefreshTime = OutTime;
        NumFrames = 0;
    }
    NumFrames++;
}

static LRESULT CALLBACK ProcessWindowMessage(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    switch (Message)
    {
    case WM_KEYDOWN:
        if (WParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
            return 0;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(Window, Message, WParam, LParam);
}

static HWND CreateSimpleWindow(const char* Name, uint32_t Width, uint32_t Height)
{
    WNDCLASS WinClass = {};
    WinClass.lpfnWndProc = ProcessWindowMessage;
    WinClass.hInstance = GetModuleHandle(nullptr);
    WinClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    WinClass.lpszClassName = Name;
    if (!RegisterClass(&WinClass))
    {
        assert(0);
    }

    RECT Rect = { 0, 0, static_cast<LONG>(Width), static_cast<LONG>(Height) };
    if (!AdjustWindowRect(&Rect, WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX, 0))
    {
        assert(0);
    }

    HWND Window = CreateWindowEx(0, Name, Name, WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, Rect.right - Rect.left, Rect.bottom - Rect.top, nullptr, nullptr, nullptr, 0);
    assert(Window);
    return Window;
}

int32_t CALLBACK WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int32_t)
{
    SetProcessDPIAware();

#ifdef _DEBUG
    const char* Name = "genexp-120819-debug";
#else
    const char* Name = "genexp-120819";
#endif
    HWND Window = CreateSimpleWindow(Name, GResolutionX, GResolutionY);
    CreateGraphicsContext(Window);

    for (;;)
    {
        MSG Message = {};
        if (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
        {
            DispatchMessage(&Message);
            if (Message.message == WM_QUIT)
            {
                break;
            }
        }
        else
        {
            double Time;
            float DeltaTime;
            UpdateFrameStats(Window, Name, Time, DeltaTime);
            Draw(static_cast<float>(Time));
            PresentFrame();
        }
    }

    WaitForGPU();
    DestroyGraphicsContext();

    return 0;
}
