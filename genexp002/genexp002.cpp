static void
FUpdate(TGenExp002& E002, TDirectX12& Dx, double Time, float DeltaTime)
{
    //ImGui::ShowDemoWindow();

    TFrameResources& Frame = E002.Frames[Dx.FrameIndex];
    {
        XMFLOAT2* Ptr = (XMFLOAT2*)Frame.PointsCpuAddress;
        *Ptr++ = XMFLOAT2(-0.5f + FRandomf(-0.05f, 0.05f), -0.5f + FRandomf(-0.05f, 0.05f));
        *Ptr++ = XMFLOAT2(0.5f + FRandomf(-0.05f, 0.05f), 0.15f + FRandomf(-0.05f, 0.05f));
    }

    Dx.CmdList->OMSetRenderTargets(1, &E002.CanvasRtv, 0, nullptr);
    Dx.CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    Dx.CmdList->SetPipelineState(E002.LinePso);
    Dx.CmdList->SetGraphicsRootSignature(E002.LineRsi);
    Dx.CmdList->IASetVertexBuffers(0, 1, &Frame.PointsVbv);
    Dx.CmdList->DrawInstanced(2, 1, 0, 0);

    Dx.CmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(E002.CanvasTex, D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                                         D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
    Dx.CmdList->OMSetRenderTargets(1, &FGetBackBufferRtv(Dx), 0, nullptr);
    Dx.CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Dx.CmdList->SetPipelineState(E002.CanvasDisplayPso);
    Dx.CmdList->SetGraphicsRootSignature(E002.CanvasDisplayRsi);
    Dx.CmdList->SetGraphicsRootDescriptorTable(0, FCopyDescriptorsToGpu(Dx, 1, E002.CanvasSrv));
    Dx.CmdList->DrawInstanced(3, 1, 0, 0);
    Dx.CmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(E002.CanvasTex,
                                                                         D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                                                                         D3D12_RESOURCE_STATE_RENDER_TARGET));
}

static void
FInitializeFrameResources(TGenExp002& E002, TDirectX12& Dx, unsigned FrameIndex)
{
    TFrameResources& Frame = E002.Frames[FrameIndex];

    // Points buffer
    {
        const unsigned KSize = 128 * 1024;
        ID3D12Resource* Buffer = E002.Resources.push_back();
        VHR(Dx.Device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
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
FInitialize(TGenExp002& E002, TDirectX12& Dx)
{
    // Canvas resources
    {
        auto ImageDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, Dx.Resolution[0], Dx.Resolution[1]);
        ImageDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        const auto ClearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16B16A16_FLOAT, XMVECTORF32{ 1.0f, 1.0f, 1.0f, 0.0f });

        VHR(Dx.Device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
                                               &ImageDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue,
                                               IID_PPV_ARGS(&E002.CanvasTex)));

        FAllocateDescriptors(Dx, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1, E002.CanvasRtv);
        Dx.Device->CreateRenderTargetView(E002.CanvasTex, nullptr, E002.CanvasRtv);

        FAllocateDescriptors(Dx, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, E002.CanvasSrv);
        Dx.Device->CreateShaderResourceView(E002.CanvasTex, nullptr, E002.CanvasSrv);

        Dx.CmdList->OMSetRenderTargets(1, &E002.CanvasRtv, 0, nullptr);
        Dx.CmdList->ClearRenderTargetView(E002.CanvasRtv, ClearValue.Color, 0, nullptr);
    }
    // CanvasDisplay shaders
    {
        eastl::vector<uint8_t> CsoVs = FLoadFile("data/shaders/display-canvas-vs.cso");
        eastl::vector<uint8_t> CsoPs = FLoadFile("data/shaders/display-canvas-ps.cso");

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

        VHR(Dx.Device->CreateGraphicsPipelineState(&PsoDesc, IID_PPV_ARGS(&E002.CanvasDisplayPso)));
        VHR(Dx.Device->CreateRootSignature(0, CsoVs.data(), CsoVs.size(), IID_PPV_ARGS(&E002.CanvasDisplayRsi)));
    }
    // Line shaders
    {
        eastl::vector<uint8_t> CsoVs = FLoadFile("data/shaders/line-vs.cso");
        eastl::vector<uint8_t> CsoPs = FLoadFile("data/shaders/line-ps.cso");

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

        VHR(Dx.Device->CreateGraphicsPipelineState(&PsoDesc, IID_PPV_ARGS(&E002.LinePso)));
        VHR(Dx.Device->CreateRootSignature(0, CsoVs.data(), CsoVs.size(), IID_PPV_ARGS(&E002.LineRsi)));
    }
    FInitializeFrameResources(E002, Dx, 0);
    FInitializeFrameResources(E002, Dx, 1);
}

static void
FShutdown(TGenExp002& E002)
{
    // @Incomplete: Release all resources.
    for (ID3D12Resource* Resource : E002.Resources)
        SAFE_RELEASE(Resource);
}
// vim: set ts=4 sw=4 expandtab:
