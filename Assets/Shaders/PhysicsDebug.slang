#include "Common.slang"

Bind(0, Uniform)
ConstantBuffer<matrix> model : register(b0, space1);
Bind(1, Uniform)
ConstantBuffer<matrix> view : register(b1, space1);
Bind(2, Uniform)
ConstantBuffer<matrix> projection : register(b2, space1);

[shader("vertex")]
float4 VertexMain(float3 position : Position, inout float3 color: Color) : SV_Position
{
    return mul(mul(mul(float4(position, 1.0), model), view), projection);
}

[shader("fragment")]
float4 FragmentMain(in float4 position : SV_Position, in float3 color: Color) : SV_Target
{
    return float4(color, 1.0);
}