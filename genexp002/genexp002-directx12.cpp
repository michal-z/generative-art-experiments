namespace Dx
{
namespace prv
{

struct TDescriptorHeap
{
    ID3D12DescriptorHeap* Heap;
    D3D12_CPU_DESCRIPTOR_HANDLE CpuStart;
    D3D12_GPU_DESCRIPTOR_HANDLE GpuStart;
    unsigned Size;
    unsigned Capacity;
};

struct TGpuMemoryHeap
{
    ID3D12Resource* Heap;
    uint8_t* CpuStart;
    D3D12_GPU_VIRTUAL_ADDRESS GpuStart;
    unsigned Size;
    unsigned Capacity;
};

static TDescriptorHeap GRenderTargetHeap;
static TDescriptorHeap GDepthStencilHeap;

// shader visible descriptor heaps
static TDescriptorHeap GShaderVisibleHeaps[2];

// non-shader visible descriptor heap
static TDescriptorHeap GNonShaderVisibleHeap;

static TGpuMemoryHeap GUploadMemoryHeaps[2];

static IDXGISwapChain3* GSwapChain;
static ID3D12Fence* GFrameFence;
static HANDLE GFrameFenceEvent;

static uint64_t GFrameCount;


static TDescriptorHeap&
FGetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type, D3D12_DESCRIPTOR_HEAP_FLAGS Flags, unsigned& OutDescriptorSize)
{
    if (Type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
    {
        OutDescriptorSize = GDescriptorSizeRtv;
        return GRenderTargetHeap;
    }
    else if (Type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
    {
        OutDescriptorSize = GDescriptorSizeRtv;
        return GDepthStencilHeap;
    }
    else if (Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    {
        OutDescriptorSize = GDescriptorSize;
        if (Flags == D3D12_DESCRIPTOR_HEAP_FLAG_NONE)
            return GNonShaderVisibleHeap;
        else if (Flags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
            return GShaderVisibleHeaps[GFrameIndex];
    }
    assert(0);
    OutDescriptorSize = 0;
    return GNonShaderVisibleHeap;
}

} // namespace prv

static void
FAllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE Type, unsigned Count, D3D12_CPU_DESCRIPTOR_HANDLE& OutFirst)
{
    unsigned DescriptorSize;
    prv::TDescriptorHeap& DescriptorHeap = prv::FGetDescriptorHeap(Type, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, DescriptorSize);

    assert((DescriptorHeap.Size + Count) < DescriptorHeap.Capacity);

    OutFirst.ptr = DescriptorHeap.CpuStart.ptr + DescriptorHeap.Size * DescriptorSize;

    DescriptorHeap.Size += Count;
}

static void
FAllocateGpuDescriptors(unsigned Count, D3D12_CPU_DESCRIPTOR_HANDLE& OutFirstCpu, D3D12_GPU_DESCRIPTOR_HANDLE& OutFirstGpu)
{
    unsigned DescriptorSize;
    prv::TDescriptorHeap& DescriptorHeap = prv::FGetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                                                   D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, DescriptorSize);

    assert((DescriptorHeap.Size + Count) < DescriptorHeap.Capacity);

    OutFirstCpu.ptr = DescriptorHeap.CpuStart.ptr + DescriptorHeap.Size * DescriptorSize;
    OutFirstGpu.ptr = DescriptorHeap.GpuStart.ptr + DescriptorHeap.Size * DescriptorSize;

    DescriptorHeap.Size += Count;
}

static D3D12_GPU_DESCRIPTOR_HANDLE
FCopyDescriptorsToGpu(unsigned Count, D3D12_CPU_DESCRIPTOR_HANDLE Source)
{
    D3D12_CPU_DESCRIPTOR_HANDLE DestinationCpu;
    D3D12_GPU_DESCRIPTOR_HANDLE DestinationGpu;
    FAllocateGpuDescriptors(Count, DestinationCpu, DestinationGpu);
    GDevice->CopyDescriptorsSimple(Count, DestinationCpu, Source, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    return DestinationGpu;
}

static inline D3D12_CPU_DESCRIPTOR_HANDLE
FGetBackBufferRtv()
{
    D3D12_CPU_DESCRIPTOR_HANDLE BackBufferRtv = prv::GRenderTargetHeap.CpuStart;
    BackBufferRtv.ptr += GBackBufferIndex * GDescriptorSizeRtv;
    return BackBufferRtv;
}

static inline void
FSetDescriptorHeap()
{
    GCmdList->SetDescriptorHeaps(1, &prv::GShaderVisibleHeaps[GFrameIndex].Heap);
}

static void*
FAllocateGpuUploadMemory(unsigned Size, D3D12_GPU_VIRTUAL_ADDRESS& OutGpuAddress)
{
    assert(Size > 0);

    if (Size & 0xff) // always align to 256 bytes
        Size = (Size + 255) & ~0xff;

    prv::TGpuMemoryHeap& UploadHeap = prv::GUploadMemoryHeaps[GFrameIndex];
    assert((UploadHeap.Size + Size) < UploadHeap.Capacity);

    void* CpuAddr = UploadHeap.CpuStart + UploadHeap.Size;
    OutGpuAddress = UploadHeap.GpuStart + UploadHeap.Size;

    UploadHeap.Size += Size;
    return CpuAddr;
}

static void
FInit()
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
                Dbg1->SetEnableGPUBasedValidation(TRUE);
            SAFE_RELEASE(Dbg);
            SAFE_RELEASE(Dbg1);
        }
    }
#endif
    if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&GDevice))))
    {
        // @Incomplete: Add MessageBox.
        return;
    }

    D3D12_COMMAND_QUEUE_DESC CmdQueueDesc = {};
    CmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    CmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    CmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    VHR(GDevice->CreateCommandQueue(&CmdQueueDesc, IID_PPV_ARGS(&GCmdQueue)));

    DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};
    SwapChainDesc.BufferCount = 4;
    SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDesc.OutputWindow = GWindow;
    SwapChainDesc.SampleDesc.Count = 1;
    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    SwapChainDesc.Windowed = TRUE;

    IDXGISwapChain* TempSwapChain;
    VHR(Factory->CreateSwapChain(GCmdQueue, &SwapChainDesc, &TempSwapChain));
    VHR(TempSwapChain->QueryInterface(IID_PPV_ARGS(&prv::GSwapChain)));
    SAFE_RELEASE(TempSwapChain);
    SAFE_RELEASE(Factory);

    RECT Rect;
    GetClientRect(GWindow, &Rect);
    GResolution[0] = (unsigned)Rect.right;
    GResolution[1] = (unsigned)Rect.bottom;

    for (unsigned Index = 0; Index < 2; ++Index)
        VHR(GDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&GCmdAlloc[Index])));

    GDescriptorSize = GDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    GDescriptorSizeRtv = GDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Render Target Descriptor Heap
    {
        prv::GRenderTargetHeap.Size = 0;
        prv::GRenderTargetHeap.Capacity = 16;
        prv::GRenderTargetHeap.CpuStart.ptr = 0;
        prv::GRenderTargetHeap.GpuStart.ptr = 0;

        D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
        HeapDesc.NumDescriptors = prv::GRenderTargetHeap.Capacity;
        HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        VHR(GDevice->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&prv::GRenderTargetHeap.Heap)));
        prv::GRenderTargetHeap.CpuStart = prv::GRenderTargetHeap.Heap->GetCPUDescriptorHandleForHeapStart();
    }

    // Depth Stencil Descriptor Heap
    {
        prv::GDepthStencilHeap.Size = 0;
        prv::GDepthStencilHeap.Capacity = 8;
        prv::GDepthStencilHeap.CpuStart.ptr = 0;
        prv::GDepthStencilHeap.GpuStart.ptr = 0;

        D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
        HeapDesc.NumDescriptors = prv::GDepthStencilHeap.Capacity;
        HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        VHR(GDevice->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&prv::GDepthStencilHeap.Heap)));
        prv::GDepthStencilHeap.CpuStart = prv::GDepthStencilHeap.Heap->GetCPUDescriptorHandleForHeapStart();
    }

    // Shader Visible Descriptor Heaps
    {
        for (unsigned Index = 0; Index < 2; ++Index)
        {
            prv::TDescriptorHeap& Heap = prv::GShaderVisibleHeaps[Index];

            Heap.Size = 0;
            Heap.Capacity = 10000;
            Heap.CpuStart.ptr = 0;
            Heap.GpuStart.ptr = 0;

            D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
            HeapDesc.NumDescriptors = Heap.Capacity;
            HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            VHR(GDevice->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&Heap.Heap)));

            Heap.CpuStart = Heap.Heap->GetCPUDescriptorHandleForHeapStart();
            Heap.GpuStart = Heap.Heap->GetGPUDescriptorHandleForHeapStart();
        }
    }

    // Non-Shader Visible Descriptor Heap
    {
        prv::GNonShaderVisibleHeap.Size = 0;
        prv::GNonShaderVisibleHeap.Capacity = 10000;
        prv::GNonShaderVisibleHeap.CpuStart.ptr = 0;
        prv::GNonShaderVisibleHeap.GpuStart.ptr = 0;

        D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
        HeapDesc.NumDescriptors = prv::GNonShaderVisibleHeap.Capacity;
        HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        VHR(GDevice->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&prv::GNonShaderVisibleHeap.Heap)));
        prv::GNonShaderVisibleHeap.CpuStart = prv::GNonShaderVisibleHeap.Heap->GetCPUDescriptorHandleForHeapStart();
    }

    // Upload Memory Heaps
    {
        for (unsigned Index = 0; Index < 2; ++Index)
        {
            prv::TGpuMemoryHeap& UploadHeap = prv::GUploadMemoryHeaps[Index];
            UploadHeap.Size = 0;
            UploadHeap.Capacity = 8*1024*1024;
            UploadHeap.CpuStart = 0;
            UploadHeap.GpuStart = 0;

            VHR(GDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
                                                 &CD3DX12_RESOURCE_DESC::Buffer(UploadHeap.Capacity),
                                                 D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                 IID_PPV_ARGS(&UploadHeap.Heap)));

            VHR(UploadHeap.Heap->Map(0, &CD3DX12_RANGE(0, 0), (void**)&UploadHeap.CpuStart));

            UploadHeap.GpuStart = UploadHeap.Heap->GetGPUVirtualAddress();
        }
    }

    // Swap buffer render targets
    {
        D3D12_CPU_DESCRIPTOR_HANDLE Handle;
        FAllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 4, Handle);

        for (unsigned Index = 0; Index < 4; ++Index)
        {
            VHR(prv::GSwapChain->GetBuffer(Index, IID_PPV_ARGS(&GSwapBuffers[Index])));

            GDevice->CreateRenderTargetView(GSwapBuffers[Index], nullptr, Handle);
            Handle.ptr += GDescriptorSizeRtv;
        }
    }

    // Depth-stencil target
    {
        auto ImageDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, GResolution[0], GResolution[1]);
        ImageDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        VHR(GDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
                                             &ImageDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                             &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.0f, 0),
                                             IID_PPV_ARGS(&GDepthBuffer)));
        D3D12_CPU_DESCRIPTOR_HANDLE Handle;
        FAllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, Handle);

        D3D12_DEPTH_STENCIL_VIEW_DESC ViewDesc = {};
        ViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
        ViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        ViewDesc.Flags = D3D12_DSV_FLAG_NONE;
        GDevice->CreateDepthStencilView(GDepthBuffer, &ViewDesc, Handle);
    }

    VHR(GDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, GCmdAlloc[0], nullptr, IID_PPV_ARGS(&GCmdList)));

    VHR(GDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&prv::GFrameFence)));
    prv::GFrameFenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
}

static void
FShutdown()
{
    // @Incomplete: Release all resources.
    SAFE_RELEASE(GCmdList);
    SAFE_RELEASE(GCmdAlloc[0]);
    SAFE_RELEASE(GCmdAlloc[1]);
    SAFE_RELEASE(prv::GRenderTargetHeap.Heap);
    SAFE_RELEASE(prv::GDepthStencilHeap.Heap);
    for (unsigned Index = 0; Index < 4; ++Index)
        SAFE_RELEASE(GSwapBuffers[Index]);
    CloseHandle(prv::GFrameFenceEvent);
    SAFE_RELEASE(prv::GFrameFence);
    SAFE_RELEASE(prv::GSwapChain);
    SAFE_RELEASE(GCmdQueue);
    SAFE_RELEASE(GDevice);
}

static void
FPresentFrame()
{
    prv::GSwapChain->Present(1, 0);
    GCmdQueue->Signal(prv::GFrameFence, ++prv::GFrameCount);

    const uint64_t GpuFrameCount = prv::GFrameFence->GetCompletedValue();

    if ((prv::GFrameCount - GpuFrameCount) >= 2)
    {
        prv::GFrameFence->SetEventOnCompletion(GpuFrameCount + 1, prv::GFrameFenceEvent);
        WaitForSingleObject(prv::GFrameFenceEvent, INFINITE);
    }

    GFrameIndex = !GFrameIndex;
    GBackBufferIndex = prv::GSwapChain->GetCurrentBackBufferIndex();

    prv::GShaderVisibleHeaps[GFrameIndex].Size = 0;
    prv::GUploadMemoryHeaps[GFrameIndex].Size = 0;
}

static void
FWaitForGpu()
{
    GCmdQueue->Signal(prv::GFrameFence, ++prv::GFrameCount);
    prv::GFrameFence->SetEventOnCompletion(prv::GFrameCount, prv::GFrameFenceEvent);
    WaitForSingleObject(prv::GFrameFenceEvent, INFINITE);
}

} // namespace Dx
// vim: set ts=4 sw=4 expandtab:
