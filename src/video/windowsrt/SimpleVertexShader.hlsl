
//#pragma pack_matrix( row_major )

struct VertexShaderInput
{
	float3 pos : POSITION;
	float3 color : COLOR0;
	float2 tex : TEXCOORD0;
};

struct VertexShaderOutput
{
	float4 pos : SV_POSITION;
	float3 color : COLOR0;
	float2 tex : TEXCOORD0;
};

VertexShaderOutput main(VertexShaderInput input)
{
	VertexShaderOutput output;
	output.pos = float4(input.pos, 1.0f);
	output.color = input.color;
	output.tex = input.tex;
	return output;
}
