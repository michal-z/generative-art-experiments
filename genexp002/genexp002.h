namespace Dx
{

static ID3D12Device* GDevice;
static ID3D12CommandQueue* GCmdQueue;
static ID3D12CommandAllocator* GCmdAlloc[2];
static ID3D12GraphicsCommandList* GCmdList;
static ID3D12Resource* GSwapBuffers[4];
static ID3D12Resource* GDepthBuffer;
static HWND GWindow;
static unsigned GResolution[2];
static unsigned GDescriptorSize;
static unsigned GDescriptorSizeRtv;
static unsigned GFrameIndex;
static unsigned GBackBufferIndex;
static eastl::vector<ID3D12Resource*> GIntermediateResources;

static void FAllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE Type,
                                 unsigned Count,
                                 D3D12_CPU_DESCRIPTOR_HANDLE& OutFirst);
static void FAllocateGpuDescriptors(unsigned Count,
                                    D3D12_CPU_DESCRIPTOR_HANDLE& OutFirstCpu,
                                    D3D12_GPU_DESCRIPTOR_HANDLE& OutFirstGpu);
static D3D12_GPU_DESCRIPTOR_HANDLE FCopyDescriptorsToGpu(unsigned Count,
                                                         D3D12_CPU_DESCRIPTOR_HANDLE Source);
static inline D3D12_CPU_DESCRIPTOR_HANDLE FGetBackBufferRtv();
static inline void FSetDescriptorHeap();
static void* FAllocateGpuUploadMemory(unsigned Size,
                                      D3D12_GPU_VIRTUAL_ADDRESS& OutGpuAddress);
static void FInit();
static void FShutdown();
static void FPresentFrame();
static void FWaitForGpu();

} // namespace Dx

namespace Gui
{

static void FInit();
static void FRender();
static void FShutdown();

} // namespace Gui

namespace Misc
{

static eastl::vector<uint8_t> FLoadFile(const char* FileName);
static double FGetTime();
static inline float FRandomf();
static inline float FRandomf(float begin, float end);

} // namespace Misc
// vim: set ts=4 sw=4 expandtab:
