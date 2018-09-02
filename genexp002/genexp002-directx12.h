struct descriptor_heap
{
    ID3D12DescriptorHeap* Heap;
    D3D12_CPU_DESCRIPTOR_HANDLE CpuStart;
    D3D12_GPU_DESCRIPTOR_HANDLE GpuStart;
    unsigned Size;
    unsigned Capacity;
};

struct gpu_memory_heap
{
    ID3D12Resource* Heap;
    uint8_t* CpuStart;
    D3D12_GPU_VIRTUAL_ADDRESS GpuStart;
    unsigned Size;
    unsigned Capacity;
};

struct directx12
{
    ID3D12Device* Device;
    ID3D12CommandQueue* CmdQueue;
    ID3D12CommandAllocator* CmdAlloc[2];
    ID3D12GraphicsCommandList* CmdList;
    IDXGISwapChain3* SwapChain;
    ID3D12Resource* SwapBuffers[4];
    ID3D12Resource* DepthBuffer;
    ID3D12Fence* FrameFence;
    HANDLE FrameFenceEvent;
    HWND Window;
    unsigned Resolution[2];
    unsigned DescriptorSize;
    unsigned DescriptorSizeRtv;
    unsigned FrameIndex;
    unsigned BackBufferIndex;
    uint64_t FrameCount;

    eastl::vector<ID3D12Resource*> IntermediateResources;

    descriptor_heap RenderTargetHeap;
    descriptor_heap DepthStencilHeap;

    // shader visible descriptor heaps
    descriptor_heap ShaderVisibleHeaps[2];

    // non-shader visible descriptor heap
    descriptor_heap NonShaderVisibleHeap;

    gpu_memory_heap UploadMemoryHeaps[2];
};
// vim: set ts=4 sw=4 expandtab:
