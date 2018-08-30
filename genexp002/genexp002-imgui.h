struct imgui_frame_resources
{
    ID3D12Resource* VertexBuffer;
    void* VertexBufferCpuAddress;
    uint32_t VertexBufferSize;
    D3D12_VERTEX_BUFFER_VIEW VertexBufferView;

    ID3D12Resource* IndexBuffer;
    void* IndexBufferCpuAddress;
    uint32_t IndexBufferSize;
    D3D12_INDEX_BUFFER_VIEW IndexBufferView;
};

struct imgui_renderer
{
    imgui_frame_resources FrameResources[2];

    ID3D12RootSignature* RootSignature;
    ID3D12PipelineState* PipelineState;
    ID3D12Resource* FontTexture;
    D3D12_CPU_DESCRIPTOR_HANDLE FontTextureDescriptor;
};
// vim: set ts=4 sw=4 expandtab:
