#if defined(SDL3GPU)
    #if defined(VERTEX)
        #define Sampler 0
        #define Uniform 1
    #elif defined(FRAGMENT)
        #define Sampler 2
        #define Uniform 3
    #endif
#else // defined(OPENGL)
    #define Uniform 1
    #define Sampler 0
#endif

#define Bind(bind, type) [vk::binding(bind, type)]

struct Vertex
{
    float3 color;
    float2 texCoord;
};

Bind(0, Uniform)
ConstantBuffer<matrix> model;
Bind(1, Uniform)
ConstantBuffer<matrix> view;
Bind(2, Uniform)
ConstantBuffer<matrix> projection;

[shader("vertex")]
float4 VertexMain(float3 position, float3 color, float2 texCoord, out Vertex data) : SV_Position
{
    data.color = color;
    data.texCoord = texCoord;

    return mul(mul(mul(float4(position, 1.0), model), view), projection);
}

Bind(0, Sampler)
Sampler2D<float4> texture_diffuse0;
Bind(1, Sampler)
Sampler2D<float4> texture_diffuse1;
Bind(2, Sampler)
Sampler2D<float4> texture_diffuse2;
Bind(3, Sampler)
Sampler2D<float4> texture_specular0;
Bind(4, Sampler)
Sampler2D<float4> texture_specular1;

[shader("fragment")]
float4 FragmentMain(float4 position: SV_Position, in Vertex data) : SV_Target
{
    return texture_diffuse0.Sample(data.texCoord);
}