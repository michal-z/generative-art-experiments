//=============================================================================
#if defined(VS_IMGUI) || defined(PS_IMGUI)
//=============================================================================

#define Rsi \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
    "CBV(b0, visibility = SHADER_VISIBILITY_VERTEX), " \
    "DescriptorTable(SRV(t0), visibility = SHADER_VISIBILITY_PIXEL), " \
    "StaticSampler(s0, filter = FILTER_MIN_MAG_MIP_LINEAR, visibility = SHADER_VISIBILITY_PIXEL)"

struct TVertexData
{
    float2 Position : POSITION;
    float2 Texcoord : TEXCOORD0;
    float4 Color : COLOR;
};

struct TPixelData
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

[RootSignature(Rsi)]
TPixelData
VsImgui(TVertexData Input)
{
    TPixelData Output;
    Output.Position = mul(float4(Input.Position, 0.0f, 1.0f), s_Cb.Matrix);
    Output.Texcoord = Input.Texcoord;
    Output.Color = Input.Color;
    return Output;
}

#elif defined(PS_IMGUI)

Texture2D s_GuiTexture : register(t0);
SamplerState s_GuiSampler : register(s0);

[RootSignature(Rsi)]
float4
PsImgui(TPixelData Input) : SV_Target0
{
    return Input.Color * s_GuiTexture.Sample(s_GuiSampler, Input.Texcoord);
}

#endif
//=============================================================================
#elif defined(VS_DISPLAY_CANVAS) || defined(PS_DISPLAY_CANVAS)
//=============================================================================

#define Rsi \
    "RootFlags(0), " \
    "DescriptorTable(SRV(t0), visibility = SHADER_VISIBILITY_PIXEL), " \
    "StaticSampler(s0, filter = FILTER_MIN_MAG_MIP_LINEAR, visibility = SHADER_VISIBILITY_PIXEL)"

struct TPixelData
{
    float4 Position : SV_Position;
    float2 Texcoord : TEXCOORD0;
};

#if defined(VS_DISPLAY_CANVAS)

[RootSignature(Rsi)]
TPixelData
VsDisplayCanvas(uint VertexId : SV_VertexID)
{
    float2 Positions[] = { float2(-1.0f, -1.0f), float2(-1.0f, 3.0f), float2(3.0f, -1.0f) };
    TPixelData Output;
    Output.Position = float4(Positions[VertexId], 0.0f, 1.0f);
    Output.Texcoord = 0.5f + 0.5f * Positions[VertexId];
    return Output;
}

#elif defined(PS_DISPLAY_CANVAS)

Texture2D s_CanvasTexture : register(t0);
SamplerState s_CanvasSampler : register(s0);

[RootSignature(Rsi)]
float4
PsDisplayCanvas(TPixelData Input) : SV_Target0
{
    return s_CanvasTexture.Sample(s_CanvasSampler, Input.Texcoord);
}

#endif
//=============================================================================
#elif defined(VS_LINE) || defined(PS_LINE)
//=============================================================================

#define Rsi \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)"

struct TVertexData
{
    float2 Position : POSITION;
};

struct TPixelData
{
    float4 Position : SV_Position;
};

#if defined(VS_LINE)

[RootSignature(Rsi)]
TPixelData
VsLine(TVertexData Input)
{
    TPixelData Output;
    Output.Position = float4(Input.Position, 0.0f, 1.0f);
    return Output;
}

#elif defined(PS_LINE)

[RootSignature(Rsi)]
float4
PsLine(TPixelData Input) : SV_Target0
{
    return float4(0.0f, 0.0f, 0.0f, 0.01f);
}

#endif
//=============================================================================
#endif
// vim: set ts=4 sw=4 expandtab:
