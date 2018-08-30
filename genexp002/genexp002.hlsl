#if defined(VS_IMGUI) || defined(PS_IMGUI)

#define RsDecl \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
    "CBV(b0, visibility = SHADER_VISIBILITY_VERTEX), " \
    "DescriptorTable(SRV(t0), visibility = SHADER_VISIBILITY_PIXEL), " \
    "StaticSampler(s0, filter = FILTER_MIN_MAG_MIP_LINEAR, visibility = SHADER_VISIBILITY_PIXEL)"

struct input_data
{
    float2 Position : POSITION;
    float2 Texcoord : TEXCOORD0;
    float4 Color : COLOR;
};

struct output_data
{
    float4 Position : SV_Position;
    float2 Texcoord : TEXCOORD0;
    float4 Color : COLOR;
};

#if defined(VS_IMGUI)

struct constant_data
{
    float4x4 Matrix;
};
ConstantBuffer<constant_data> s_Cb : register(b0);

[RootSignature(RsDecl)]
output_data VsImgui(input_data Input)
{
    output_data Output;
    Output.Position = mul(float4(Input.Position, 0.0f, 1.0f), s_Cb.Matrix);
    Output.Texcoord = Input.Texcoord;
    Output.Color = Input.Color;
    return Output;
}

#elif defined(PS_IMGUI)

Texture2D s_ImGuiTexture : register(t0);
SamplerState s_ImGuiSampler : register(s0);

[RootSignature(RsDecl)]
float4 PsImgui(output_data Input) : SV_Target0
{
    return Input.Color * s_ImGuiTexture.Sample(s_ImGuiSampler, Input.Texcoord);
}

#endif
#endif // #if defined(VS_IMGUI) || defined(PS_IMGUI)
// vim: set ts=4 sw=4 expandtab:
