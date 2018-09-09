#if defined(VS_IMGUI) || defined(PS_IMGUI)

#define RsDecl \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
    "CBV(b0, visibility = SHADER_VISIBILITY_VERTEX), " \
    "DescriptorTable(SRV(t0), visibility = SHADER_VISIBILITY_PIXEL), " \
    "StaticSampler(s0, filter = FILTER_MIN_MAG_MIP_LINEAR, visibility = SHADER_VISIBILITY_PIXEL)"

struct TInputData
{
    float2 Position : POSITION;
    float2 Texcoord : TEXCOORD0;
    float4 Color : COLOR;
};

struct TOutputData
{
    float4 Position : SV_Position;
    float2 Texcoord : TEXCOORD0;
    float4 Color : COLOR;
};

#if defined(VS_IMGUI)

struct TConstantData
{
    float4x4 Matrix;
};
ConstantBuffer<TConstantData> s_Cb : register(b0);

[RootSignature(RsDecl)] TOutputData
VsImgui(TInputData Input)
{
    TOutputData Output;
    Output.Position = mul(float4(Input.Position, 0.0f, 1.0f), s_Cb.Matrix);
    Output.Texcoord = Input.Texcoord;
    Output.Color = Input.Color;
    return Output;
}

#elif defined(PS_IMGUI)

Texture2D s_GuiTexture : register(t0);
SamplerState s_GuiSampler : register(s0);

[RootSignature(RsDecl)] float4
PsImgui(TOutputData Input) : SV_Target0
{
    return Input.Color * s_GuiTexture.Sample(s_GuiSampler, Input.Texcoord);
}

#endif
#elif defined(VS_FULL_TRIANGLE) || defined(PS_DISPLAY_CANVAS)

#define RsDecl \
    "RootFlags(0), " \
    "DescriptorTable(SRV(t0), visibility = SHADER_VISIBILITY_PIXEL), " \
    "StaticSampler(s0, filter = FILTER_MIN_MAG_MIP_LINEAR, visibility = SHADER_VISIBILITY_PIXEL)"

struct TOutputData
{
    float4 Position : SV_Position;
    float2 Texcoord : TEXCOORD0;
};

#if defined(VS_FULL_TRIANGLE)

[RootSignature(RsDecl)] TOutputData
VsFullTriangle(uint VertexId : SV_VertexID)
{
    float2 Positions[] = { float2(-1.0f, -1.0f), float2(-1.0f, 3.0f), float2(3.0f, -1.0f) };
    TOutputData Output;
    Output.Position = float4(Positions[VertexId], 0.0f, 1.0f);
    Output.Texcoord = 0.5f + 0.5f * Positions[VertexId];
    return Output;
}

#elif defined(PS_DISPLAY_CANVAS)

Texture2D s_CanvasTexture : register(t0);
SamplerState s_CanvasSampler : register(s0);

[RootSignature(RsDecl)] float4
PsDisplayCanvas(TOutputData Input) : SV_Target0
{
    return s_CanvasTexture.Sample(s_CanvasSampler, Input.Texcoord);
}

#endif
#else
#error "define shader to compile"
#endif
// vim: set ts=4 sw=4 expandtab:
