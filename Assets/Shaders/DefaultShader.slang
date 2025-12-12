#include "Common.slang"

struct Vertex
{
    float3 position;
    float3 color;
    float2 tex_coord;
};

Bind(0, Uniform)
ConstantBuffer<matrix> model : register(b0, space1);
Bind(1, Uniform)
ConstantBuffer<matrix> view : register(b1, space1);
Bind(2, Uniform)
ConstantBuffer<matrix> projection : register(b2, space1);

[shader("vertex")]
float4 VertexMain(in out Vertex vertex) : SV_Position
{
    return mul(mul(mul(float4(vertex.position, 1.0), model), view), projection);
}

Bind(0, Sampler)
Sampler2D<float4> texture_diffuse0 : register(t0, space2);

[shader("fragment")]
float4 FragmentMain(in Vertex data) : SV_Target
{
    return texture_diffuse0.Sample(data.tex_coord);
}