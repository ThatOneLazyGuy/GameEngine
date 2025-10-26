#include "Common.slang"

Bind(0, Uniform)
ConstantBuffer<matrix> model : register(b0, space1);
Bind(1, Uniform)
ConstantBuffer<matrix> view : register(b1, space1);
Bind(2, Uniform)
ConstantBuffer<matrix> projection : register(b2, space1);

[shader("vertex")]
float4 VertexMain(in float3 position: TEXCOORD0, inout float3 color: TEXCOORD1) : SV_Position
{
    return mul(mul(mul(float4(position, 1.0), model), view), projection);
}

[shader("fragment")]
float4 FragmentMain(in float4 position : SV_Position, in float3 color: TEXCOORD1) : SV_Target
{
    return float4(color, 1.0);
}