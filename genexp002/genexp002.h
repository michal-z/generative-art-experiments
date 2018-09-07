#include "genexp002-directx12.h"
#include "genexp002-imgui.h"

struct TGenExp002
{
    eastl::vector<ID3D12Resource*> Dx12Resources;
    D3D12_CPU_DESCRIPTOR_HANDLE CanvasDescriptor;
};
// vim: set ts=4 sw=4 expandtab:
