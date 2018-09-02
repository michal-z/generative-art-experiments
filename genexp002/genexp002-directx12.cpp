static void
InitializeDirectX12(directx12& Dx)
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
    if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&Dx.Device))))
    {
        // @Incomplete: Add MessageBox.
        return;
    }

    D3D12_COMMAND_QUEUE_DESC CmdQueueDesc = {};
    CmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    CmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    CmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    VHR(Dx.Device->CreateCommandQueue(&CmdQueueDesc, IID_PPV_ARGS(&Dx.CmdQueue)));

    DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};
    SwapChainDesc.BufferCount = 4;
    SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDesc.OutputWindow = Dx.Window;
    SwapChainDesc.SampleDesc.Count = 1;
    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    SwapChainDesc.Windowed = TRUE;

    IDXGISwapChain* TempSwapChain;
    VHR(Factory->CreateSwapChain(Dx.CmdQueue, &SwapChainDesc, &TempSwapChain));
    VHR(TempSwapChain->QueryInterface(IID_PPV_ARGS(&Dx.SwapChain)));
    SAFE_RELEASE(TempSwapChain);
    SAFE_RELEASE(Factory);

    RECT Rect;
    GetClientRect(Dx.Window, &Rect);
    Dx.Resolution[0] = (uint32_t)Rect.right;
    Dx.Resolution[1] = (uint32_t)Rect.bottom;

    for (uint32_t Index = 0; Index < 2; ++Index)
        VHR(Dx.Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&Dx.CmdAlloc[Index])));

    Dx.DescriptorSize = Dx.Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    Dx.DescriptorSizeRtv = Dx.Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Render Target Descriptor Heap
    {
        Dx.RenderTargetHeap.Size = 0;
        Dx.RenderTargetHeap.Capacity = 16;
        Dx.RenderTargetHeap.CpuStart.ptr = 0;
        Dx.RenderTargetHeap.GpuStart.ptr = 0;

        D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
        HeapDesc.NumDescriptors = Dx.RenderTargetHeap.Capacity;
        HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        VHR(Dx.Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&Dx.RenderTargetHeap.Heap)));
        Dx.RenderTargetHeap.CpuStart = Dx.RenderTargetHeap.Heap->GetCPUDescriptorHandleForHeapStart();

        D3D12_CPU_DESCRIPTOR_HANDLE Handle = Dx.RenderTargetHeap.CpuStart;

        for (uint32_t Index = 0; Index < 4; ++Index)
        {
            VHR(Dx.SwapChain->GetBuffer(Index, IID_PPV_ARGS(&Dx.SwapBuffers[Index])));

            Dx.Device->CreateRenderTargetView(Dx.SwapBuffers[Index], nullptr, Handle);
            Handle.ptr += Dx.DescriptorSizeRtv;
        }
    }
    // Depth Stencil Descriptor Heap
    {
        Dx.DepthStencilHeap.Size = 0;
        Dx.DepthStencilHeap.Capacity = 8;
        Dx.DepthStencilHeap.CpuStart.ptr = 0;
        Dx.DepthStencilHeap.GpuStart.ptr = 0;

        D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
        HeapDesc.NumDescriptors = Dx.DepthStencilHeap.Capacity;
        HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        VHR(Dx.Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&Dx.DepthStencilHeap.Heap)));
        Dx.DepthStencilHeap.CpuStart = Dx.DepthStencilHeap.Heap->GetCPUDescriptorHandleForHeapStart();

        auto ImageDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, Dx.Resolution[0], Dx.Resolution[1]);
        ImageDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        VHR(Dx.Device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
                                               &ImageDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                               &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.0f, 0),
                                               IID_PPV_ARGS(&Dx.DepthBuffer)));

        D3D12_DEPTH_STENCIL_VIEW_DESC ViewDesc = {};
        ViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
        ViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        ViewDesc.Flags = D3D12_DSV_FLAG_NONE;
        Dx.Device->CreateDepthStencilView(Dx.DepthBuffer, &ViewDesc, Dx.DepthStencilHeap.CpuStart);
    }
    // Shader Visible Descriptor Heaps
    {
        for (uint32_t Index = 0; Index < 2; ++Index)
        {
            Dx.ShaderVisibleHeaps[Index].Size = 0;
            Dx.ShaderVisibleHeaps[Index].Capacity = 10000;
            Dx.ShaderVisibleHeaps[Index].CpuStart.ptr = 0;
            Dx.ShaderVisibleHeaps[Index].GpuStart.ptr = 0;

            D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
            HeapDesc.NumDescriptors = Dx.ShaderVisibleHeaps[Index].Capacity;
            HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            VHR(Dx.Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&Dx.ShaderVisibleHeaps[Index].Heap)));

            Dx.ShaderVisibleHeaps[Index].CpuStart = Dx.ShaderVisibleHeaps[Index].Heap->GetCPUDescriptorHandleForHeapStart();
            Dx.ShaderVisibleHeaps[Index].GpuStart = Dx.ShaderVisibleHeaps[Index].Heap->GetGPUDescriptorHandleForHeapStart();
        }
    }
    // Non-Shader Visible Descriptor Heap
    {
        Dx.NonShaderVisibleHeap.Size = 0;
        Dx.NonShaderVisibleHeap.Capacity = 10000;
        Dx.NonShaderVisibleHeap.CpuStart.ptr = 0;
        Dx.NonShaderVisibleHeap.GpuStart.ptr = 0;

        D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
        HeapDesc.NumDescriptors = Dx.NonShaderVisibleHeap.Capacity;
        HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        VHR(Dx.Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&Dx.NonShaderVisibleHeap.Heap)));
        Dx.NonShaderVisibleHeap.CpuStart = Dx.NonShaderVisibleHeap.Heap->GetCPUDescriptorHandleForHeapStart();
    }
    // Upload Memory Heaps
    {
        for (uint32_t Index = 0; Index < 2; ++Index)
        {
            gpu_memory_heap& UploadHeap = Dx.UploadMemoryHeaps[Index];
            UploadHeap.Size = 0;
            UploadHeap.Capacity = 8*1024*1024;
            UploadHeap.CpuStart = 0;
            UploadHeap.GpuStart = 0;

            VHR(Dx.Device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
                                                   &CD3DX12_RESOURCE_DESC::Buffer(UploadHeap.Capacity),
                                                   D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                   IID_PPV_ARGS(&UploadHeap.Heap)));

            VHR(UploadHeap.Heap->Map(0, &CD3DX12_RANGE(0, 0), (void**)&UploadHeap.CpuStart));

            UploadHeap.GpuStart = UploadHeap.Heap->GetGPUVirtualAddress();
        }
    }

    VHR(Dx.Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, Dx.CmdAlloc[0], nullptr, IID_PPV_ARGS(&Dx.CmdList)));

    VHR(Dx.Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Dx.FrameFence)));
    Dx.FrameFenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
}

static void
ShutdownDirectX12(directx12& Dx)
{
    // @Incomplete: Release all resources.
    SAFE_RELEASE(Dx.CmdList);
    SAFE_RELEASE(Dx.CmdAlloc[0]);
    SAFE_RELEASE(Dx.CmdAlloc[1]);
    SAFE_RELEASE(Dx.RenderTargetHeap.Heap);
    SAFE_RELEASE(Dx.DepthStencilHeap.Heap);
    for (int Index = 0; Index < 4; ++Index)
        SAFE_RELEASE(Dx.SwapBuffers[Index]);
    CloseHandle(Dx.FrameFenceEvent);
    SAFE_RELEASE(Dx.FrameFence);
    SAFE_RELEASE(Dx.SwapChain);
    SAFE_RELEASE(Dx.CmdQueue);
    SAFE_RELEASE(Dx.Device);
}

static void
PresentFrame(directx12& Dx)
{
    Dx.SwapChain->Present(0, 0);
    Dx.CmdQueue->Signal(Dx.FrameFence, ++Dx.FrameCount);

    const uint64_t GpuFrameCount = Dx.FrameFence->GetCompletedValue();

    if ((Dx.FrameCount - GpuFrameCount) >= 2)
    {
        Dx.FrameFence->SetEventOnCompletion(GpuFrameCount + 1, Dx.FrameFenceEvent);
        WaitForSingleObject(Dx.FrameFenceEvent, INFINITE);
    }

    Dx.FrameIndex = !Dx.FrameIndex;
    Dx.BackBufferIndex = Dx.SwapChain->GetCurrentBackBufferIndex();

    Dx.ShaderVisibleHeaps[Dx.FrameIndex].Size = 0;
    Dx.UploadMemoryHeaps[Dx.FrameIndex].Size = 0;
}

static void
WaitForGpu(directx12& Dx)
{
    Dx.CmdQueue->Signal(Dx.FrameFence, ++Dx.FrameCount);
    Dx.FrameFence->SetEventOnCompletion(Dx.FrameCount, Dx.FrameFenceEvent);
    WaitForSingleObject(Dx.FrameFenceEvent, INFINITE);
}

static descriptor_heap&
GetDescriptorHeap(directx12& Dx, D3D12_DESCRIPTOR_HEAP_TYPE Type, D3D12_DESCRIPTOR_HEAP_FLAGS Flags,
                  uint32_t& OutDescriptorSize)
{
    if (Type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
    {
        OutDescriptorSize = Dx.DescriptorSizeRtv;
        return Dx.RenderTargetHeap;
    }
    else if (Type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
    {
        OutDescriptorSize = Dx.DescriptorSizeRtv;
        return Dx.DepthStencilHeap;
    }
    else if (Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    {
        OutDescriptorSize = Dx.DescriptorSize;
        if (Flags == D3D12_DESCRIPTOR_HEAP_FLAG_NONE)
            return Dx.NonShaderVisibleHeap;
        else if (Flags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
            return Dx.ShaderVisibleHeaps[Dx.FrameIndex];
    }
    assert(0);
    OutDescriptorSize = 0;
    return Dx.NonShaderVisibleHeap;
}

static void
AllocateDescriptors(directx12& Dx, D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t Count, D3D12_CPU_DESCRIPTOR_HANDLE& OutFirst)
{
    uint32_t DescriptorSize;
    descriptor_heap& DescriptorHeap = GetDescriptorHeap(Dx, Type, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, DescriptorSize);

    assert((DescriptorHeap.Size + Count) < DescriptorHeap.Capacity);

    OutFirst.ptr = DescriptorHeap.CpuStart.ptr + DescriptorHeap.Size * DescriptorSize;

    DescriptorHeap.Size += Count;
}

static void
AllocateGpuDescriptors(directx12& Dx, uint32_t Count,
                       D3D12_CPU_DESCRIPTOR_HANDLE& OutFirstCpu,
                       D3D12_GPU_DESCRIPTOR_HANDLE& OutFirstGpu)
{
    uint32_t DescriptorSize;
    descriptor_heap& DescriptorHeap = GetDescriptorHeap(Dx, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                                        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, DescriptorSize);

    assert((DescriptorHeap.Size + Count) < DescriptorHeap.Capacity);

    OutFirstCpu.ptr = DescriptorHeap.CpuStart.ptr + DescriptorHeap.Size * DescriptorSize;
    OutFirstGpu.ptr = DescriptorHeap.GpuStart.ptr + DescriptorHeap.Size * DescriptorSize;

    DescriptorHeap.Size += Count;
}

static void*
AllocateGpuUploadMemory(directx12& Dx, uint32_t Size, D3D12_GPU_VIRTUAL_ADDRESS& OutGpuAddress)
{
    assert(Size > 0);

    if (Size & 0xff) // always align to 256 bytes
        Size = (Size + 255) & ~0xff;

    gpu_memory_heap& UploadHeap = Dx.UploadMemoryHeaps[Dx.FrameIndex];
    assert((UploadHeap.Size + Size) < UploadHeap.Capacity);

    void* CpuAddr = UploadHeap.CpuStart + UploadHeap.Size;
    OutGpuAddress = UploadHeap.GpuStart + UploadHeap.Size;

    UploadHeap.Size += Size;
    return CpuAddr;
}
// vim: set ts=4 sw=4 expandtab:
