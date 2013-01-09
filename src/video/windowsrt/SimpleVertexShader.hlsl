
//#pragma pack_matrix( row_major )

struct VertexShaderInput
{
    float3 pos : POSITION;
    float2 tex : TEXCOORD0;
};

struct VertexShaderOutput
{
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD0;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    output.pos = float4(input.pos, 1.0f);
    output.tex = input.tex;
    return output;
}
