struct TGuiFrameResources
{
    ID3D12Resource* VertexBuffer;
    void* VertexBufferCpuAddress;
    unsigned VertexBufferSize;
    D3D12_VERTEX_BUFFER_VIEW VertexBufferView;

    ID3D12Resource* IndexBuffer;
    void* IndexBufferCpuAddress;
    unsigned IndexBufferSize;
    D3D12_INDEX_BUFFER_VIEW IndexBufferView;
};

struct TGuiRenderer
{
    TGuiFrameResources FrameResources[2];

    ID3D12RootSignature* RootSignature;
    ID3D12PipelineState* PipelineState;
    ID3D12Resource* FontTexture;
    D3D12_CPU_DESCRIPTOR_HANDLE FontTextureDescriptor;
};
// vim: set ts=4 sw=4 expandtab:
