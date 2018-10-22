namespace GenExp002
{
namespace prv
{

struct TFrameResources
{
    D3D12_VERTEX_BUFFER_VIEW PointsVbv;
    void* PointsCpuAddress;
};

static eastl::vector<ID3D12Resource*> GResources;

static ID3D12Resource* GCanvasTex;
static D3D12_CPU_DESCRIPTOR_HANDLE GCanvasRtv;
static D3D12_CPU_DESCRIPTOR_HANDLE GCanvasSrv;
static ID3D12PipelineState* GCanvasDisplayPso;
static ID3D12RootSignature* GCanvasDisplayRsi;

static ID3D12PipelineState* GSandPso;
static ID3D12RootSignature* GSandRsi;

static TFrameResources GFrames[2];

static void
FUpdate(double Time, float DeltaTime)
{
    //ImGui::ShowDemoWindow();

    TFrameResources& Frame = GFrames[Dx::GFrameIndex];
    {
        XMFLOAT2* Ptr = (XMFLOAT2*)Frame.PointsCpuAddress;
        *Ptr++ = XMFLOAT2(-0.5f + Misc::FRandomf(-0.05f, 0.05f), -0.5f + Misc::FRandomf(-0.05f, 0.05f));
        *Ptr++ = XMFLOAT2(0.5f + Misc::FRandomf(-0.05f, 0.05f), 0.15f + Misc::FRandomf(-0.05f, 0.05f));
    }

    Dx::GCmdList->OMSetRenderTargets(1, &GCanvasRtv, 0, nullptr);
    Dx::GCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    Dx::GCmdList->SetPipelineState(GSandPso);
    Dx::GCmdList->SetGraphicsRootSignature(GSandRsi);
    Dx::GCmdList->IASetVertexBuffers(0, 1, &Frame.PointsVbv);
    Dx::GCmdList->DrawInstanced(2, 1, 0, 0);

    Dx::GCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GCanvasTex, D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                                           D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
    Dx::GCmdList->OMSetRenderTargets(1, &Dx::FGetBackBufferRtv(), 0, nullptr);
    Dx::GCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Dx::GCmdList->SetPipelineState(GCanvasDisplayPso);
    Dx::GCmdList->SetGraphicsRootSignature(GCanvasDisplayRsi);
    Dx::GCmdList->SetGraphicsRootDescriptorTable(0, Dx::FCopyDescriptorsToGpu(1, GCanvasSrv));
    Dx::GCmdList->DrawInstanced(3, 1, 0, 0);
    Dx::GCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GCanvasTex,
                                                                           D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                                                                           D3D12_RESOURCE_STATE_RENDER_TARGET));
}

static void
FInitFrameResources(unsigned FrameIndex)
{
    TFrameResources& Frame = GFrames[FrameIndex];

    // Points buffer
    {
        const unsigned KSize = 128 * 1024;
        ID3D12Resource* Buffer = GResources.push_back();
        VHR(Dx::GDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
                                                 &CD3DX12_RESOURCE_DESC::Buffer(KSize),
                                                 D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                 IID_PPV_ARGS(&Buffer)));

        VHR(Buffer->Map(0, &CD3DX12_RANGE(0, 0), &Frame.PointsCpuAddress));

        Frame.PointsVbv.BufferLocation = Buffer->GetGPUVirtualAddress();
        Frame.PointsVbv.StrideInBytes = sizeof(XMFLOAT2);
        Frame.PointsVbv.SizeInBytes = KSize;
    }
}

static void
FInitResources()
{
    // Canvas resources
    {
        auto ImageDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, Dx::GResolution[0], Dx::GResolution[1]);
        ImageDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        const auto ClearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16B16A16_FLOAT, XMVECTORF32{ 1.0f, 1.0f, 1.0f, 0.0f });

        VHR(Dx::GDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
                                                 &ImageDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue,
                                                 IID_PPV_ARGS(&GCanvasTex)));

        Dx::FAllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1, GCanvasRtv);
        Dx::GDevice->CreateRenderTargetView(GCanvasTex, nullptr, GCanvasRtv);

        Dx::FAllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, GCanvasSrv);
        Dx::GDevice->CreateShaderResourceView(GCanvasTex, nullptr, GCanvasSrv);

        Dx::GCmdList->OMSetRenderTargets(1, &GCanvasRtv, 0, nullptr);
        Dx::GCmdList->ClearRenderTargetView(GCanvasRtv, ClearValue.Color, 0, nullptr);
    }

    // CanvasDisplay shaders
    {
        eastl::vector<uint8_t> CsoVs = Misc::FLoadFile("data/shaders/display-canvas-vs.cso");
        eastl::vector<uint8_t> CsoPs = Misc::FLoadFile("data/shaders/display-canvas-ps.cso");

        D3D12_GRAPHICS_PIPELINE_STATE_DESC PsoDesc = {};
        PsoDesc.VS = { CsoVs.data(), CsoVs.size() };
        PsoDesc.PS = { CsoPs.data(), CsoPs.size() };
        PsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        PsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        PsoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        PsoDesc.SampleMask = UINT_MAX;
        PsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        PsoDesc.NumRenderTargets = 1;
        PsoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        PsoDesc.SampleDesc.Count = 1;

        VHR(Dx::GDevice->CreateGraphicsPipelineState(&PsoDesc, IID_PPV_ARGS(&GCanvasDisplayPso)));
        VHR(Dx::GDevice->CreateRootSignature(0, CsoVs.data(), CsoVs.size(), IID_PPV_ARGS(&GCanvasDisplayRsi)));
    }

    // Line shaders
    {
        eastl::vector<uint8_t> CsoVs = Misc::FLoadFile("data/shaders/sand-vs.cso");
        eastl::vector<uint8_t> CsoPs = Misc::FLoadFile("data/shaders/sand-ps.cso");

        D3D12_INPUT_ELEMENT_DESC InputElements[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };
        D3D12_GRAPHICS_PIPELINE_STATE_DESC PsoDesc = {};
        PsoDesc.InputLayout = { InputElements, (UINT)eastl::size(InputElements) };
        PsoDesc.VS = { CsoVs.data(), CsoVs.size() };
        PsoDesc.PS = { CsoPs.data(), CsoPs.size() };
        PsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        PsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        PsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        PsoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
        PsoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        PsoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        PsoDesc.SampleMask = UINT_MAX;
        PsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        PsoDesc.NumRenderTargets = 1;
        PsoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
        PsoDesc.SampleDesc.Count = 1;

        VHR(Dx::GDevice->CreateGraphicsPipelineState(&PsoDesc, IID_PPV_ARGS(&GSandPso)));
        VHR(Dx::GDevice->CreateRootSignature(0, CsoVs.data(), CsoVs.size(), IID_PPV_ARGS(&GSandRsi)));
    }
}

static void
FShutdown()
{
    // @Incomplete: Release all resources.
    for (ID3D12Resource* Resource : GResources)
        SAFE_RELEASE(Resource);
}

} // namespace prv

static void
FUpdate(double Time, float DeltaTime)
{
    prv::FUpdate(Time, DeltaTime);
}

static void
FInit()
{
    prv::FInitResources();
    prv::FInitFrameResources(0);
    prv::FInitFrameResources(1);
}

static void
FShutdown()
{
    prv::FShutdown();
}

} // namespace GenExp002
// vim: set ts=4 sw=4 expandtab:
