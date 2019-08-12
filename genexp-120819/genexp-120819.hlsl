#define GRootSignature \
    "DescriptorTable(UAV(u0)), " \
	"RootConstants(b0, num32BitConstants = 1)"

struct FConstantData
{
	float Time;
};
ConstantBuffer<FConstantData> G : register(b0);
RWTexture2D<float4> GImage : register(u0);

[RootSignature(GRootSignature)]
[numthreads(8, 8, 1)]
void MainCS(uint3 GlobalID : SV_DispatchThreadID)
{
	float2 Resolution;
	GImage.GetDimensions(Resolution.x, Resolution.y);

	float2 P = GlobalID.xy / Resolution;
    GImage[GlobalID.xy] = float4(P, 0.5f + 0.5f * sin(G.Time), 1.0f);
}
