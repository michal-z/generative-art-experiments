static void
Update(TGenExp002& E002, TDirectX12& Dx, double Time, float DeltaTime)
{
    ImGui::ShowDemoWindow();

    Dx.CmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(E002.CanvasTex, D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                                         D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
    Dx.CmdList->OMSetRenderTargets(1, &GetBackBufferRtv(Dx), 0, nullptr);
    Dx.CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Dx.CmdList->SetPipelineState(E002.CanvasDisplayPso);
    Dx.CmdList->SetGraphicsRootSignature(E002.CanvasDisplayRsi);
    Dx.CmdList->SetGraphicsRootDescriptorTable(0, CopyDescriptorsToGpu(Dx, 1, E002.CanvasSrv));
    Dx.CmdList->DrawInstanced(3, 1, 0, 0);
    Dx.CmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(E002.CanvasTex,
                                                                         D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                                                                         D3D12_RESOURCE_STATE_RENDER_TARGET));
}

static void
Initialize(TGenExp002& E002, TDirectX12& Dx)
{
    // Canvas resources
    {
        auto ImageDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, Dx.Resolution[0], Dx.Resolution[1]);
        ImageDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        const auto ClearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16B16A16_FLOAT, XMVECTORF32{ 1.0f, 1.0f, 1.0f, 0.0f });

        VHR(Dx.Device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
                                               &ImageDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue,
                                               IID_PPV_ARGS(&E002.CanvasTex)));

        AllocateDescriptors(Dx, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1, E002.CanvasRtv);
        Dx.Device->CreateRenderTargetView(E002.CanvasTex, nullptr, E002.CanvasRtv);

        AllocateDescriptors(Dx, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, E002.CanvasSrv);
        Dx.Device->CreateShaderResourceView(E002.CanvasTex, nullptr, E002.CanvasSrv);

        Dx.CmdList->OMSetRenderTargets(1, &E002.CanvasRtv, 0, nullptr);
        Dx.CmdList->ClearRenderTargetView(E002.CanvasRtv, ClearValue.Color, 0, nullptr);
    }
    // CanvasDisplay shaders
    {
        eastl::vector<uint8_t> CsoVs = LoadFile("data/shaders/full-triangle-vs.cso");
        eastl::vector<uint8_t> CsoPs = LoadFile("data/shaders/display-canvas-ps.cso");

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
    }
}

static void
Shutdown(TGenExp002& E002, TDirectX12& Dx)
{
    WaitForGpu(Dx);
}
// vim: set ts=4 sw=4 expandtab:
