#include "genexp002-directx12.h"
#include "genexp002-imgui.h"

struct TGenExp002
{
    ID3D12Resource* CanvasTex;
    D3D12_CPU_DESCRIPTOR_HANDLE CanvasRtv;
    D3D12_CPU_DESCRIPTOR_HANDLE CanvasSrv;
    ID3D12PipelineState* CanvasDisplayPso;
    ID3D12RootSignature* CanvasDisplayRsi;

    ID3D12PipelineState* LinePso;
    ID3D12RootSignature* LineRsi;

    ID3D12Resource* PointBuf[2];
};
// vim: set ts=4 sw=4 expandtab:
