Texture2D theTexture : register(t0);
SamplerState theSampler : register(s0);

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 color : COLOR0;
	float2 tex : TEXCOORD0;
};

float4 main(PixelShaderInput input) : SV_TARGET
{
	//return float4(input.color,1.0f);
	return theTexture.Sample(theSampler, input.tex);
}
