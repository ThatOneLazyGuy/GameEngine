struct SharedVertexData
{
    float3 color;
    float2 texCoord;
}

[[vk::binding(0, 1)]]
ConstantBuffer<matrix> mvp;

[shader("vertex")]
float4 VertexMain(float3 position, float3 color, float2 texCoord, out SharedVertexData data) : SV_Position
{
    data.color = color;
    data.texCoord = texCoord;

    return mul(float4(position, 1.0), mvp);
}

// use vulkan binding
#if 0
[[vk::binding(0, 2)]]
Sampler2D<float4> texture_diffuse0;
[[vk::binding(1, 2)]]
Sampler2D<float4> texture_diffuse1;
[[vk::binding(2, 2)]]
Sampler2D<float4> texture_diffuse2;
[[vk::binding(3, 2)]]
Sampler2D<float4> texture_specular0;
[[vk::binding(4, 2)]]
Sampler2D<float4> texture_specular1;
#else // else OpenGL binding
[gl_binding(0)]
Sampler2D<float4> texture_diffuse0;
[gl_binding(1)]
Sampler2D<float4> texture_diffuse1;
[gl_binding(2)]
Sampler2D<float4> texture_diffuse2;
[gl_binding(3)]
Sampler2D<float4> texture_specular0;
[gl_binding(4)]
Sampler2D<float4> texture_specular1;
#endif

[shader("fragment")]
float4 FragmentMain(float4 position: SV_Position, in SharedVertexData data) : SV_Target
{
    return texture_diffuse0.Sample(data.texCoord);
}