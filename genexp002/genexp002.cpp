static void
Update(TGenExp002& E002, TDirectX12& Dx, double Time, float DeltaTime)
{
	ImGui::ShowDemoWindow();
}

static void
Initialize(TGenExp002& E002, TDirectX12& Dx)
{
    // Canvas
    {
        auto ImageDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, Dx.Resolution[0], Dx.Resolution[1]);
        ImageDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        ID3D12Resource* Resource = E002.Dx12Resources.push_back();
        VHR(Dx.Device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
                                               &ImageDesc, D3D12_RESOURCE_STATE_RENDER_TARGET,
                                               &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16B16A16_FLOAT, XMVECTORF32{ 0.0f }),
                                               IID_PPV_ARGS(&Resource)));

        AllocateDescriptors(Dx, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1, E002.CanvasDescriptor);
        Dx.Device->CreateRenderTargetView(Resource, nullptr, E002.CanvasDescriptor);
    }
}

static void
Shutdown(TGenExp002& E002, TDirectX12& Dx)
{
	WaitForGpu(Dx);
    for (ID3D12Resource* Resource : E002.Dx12Resources)
        SAFE_RELEASE(Resource);
}
// vim: set ts=4 sw=4 expandtab:
