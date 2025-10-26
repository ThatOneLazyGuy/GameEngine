#include "Common.slang"

struct Vertex
{
    float3 color;
    float2 texCoord;
};

Bind(0, Uniform)
ConstantBuffer<matrix> model : register(b0, space1);
Bind(1, Uniform)
ConstantBuffer<matrix> view : register(b1, space1);
Bind(2, Uniform)
ConstantBuffer<matrix> projection : register(b2, space1);

[shader("vertex")]
float4 VertexMain(in float3 position: TEXCOORD0, in float3 color: TEXCOORD1, in float2 texCoord: TEXCOORD2, out Vertex data: TEXCOORD3) : SV_Position
{
    data.color = color;
    data.texCoord = texCoord;

    return mul(mul(mul(float4(position, 1.0), model), view), projection);
}

Bind(0, Sampler)
Sampler2D<float4> texture_diffuse0 : register(t0, space2);
Bind(1, Sampler)
Sampler2D<float4> texture_diffuse1 : register(t1, space2);
Bind(2, Sampler)
Sampler2D<float4> texture_diffuse2 : register(t2, space2);
Bind(3, Sampler)
Sampler2D<float4> texture_specular0 : register(t3, space2);
Bind(4, Sampler)
Sampler2D<float4> texture_specular1 : register(t4, space2);

[shader("fragment")]
float4 FragmentMain(in float4 position: SV_Position, in Vertex data: TEXCOORD3) : SV_Target
{
    return texture_diffuse0.Sample(data.texCoord);
    //return float4(data.color, 1.0);
}