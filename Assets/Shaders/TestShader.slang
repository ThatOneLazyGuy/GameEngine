struct SharedVertexData
{
    float3 color;
    float2 texCoord;
}

//[gl_binding(0)]
[[vk::binding(0, 1)]]
ConstantBuffer<matrix> mvp;

[shader("vertex")]
float4 VertexMain(float3 position, float3 color, float2 texCoord, out SharedVertexData data) : SV_Position
{
    data.color = color;
    data.texCoord = texCoord;

    return mul(float4(position, 1.0), mvp);
}

[gl_binding(0)]
Sampler2D<float4> container;
[gl_binding(1)]
Sampler2D<float4> face;

[shader("fragment")]
float4 FragmentMain(float4 position: SV_Position, in SharedVertexData data) : SV_Target
{
    //return lerp(container.Sample(data.texCoord), face.Sample(data.texCoord), 1.0);
    return float4(data.color, 1.0);
}

// struct Input
// {
//     uint VertexIndex : SV_VertexID;
// };
//
// struct Output
// {
//     float4 Color : TEXCOORD0;
//     float4 Position : SV_Position;
// };
//
// [shader("vertex")]
// Output VertexMain(Input input)
// {
//     Output output;
//     float2 pos;
//     if (input.VertexIndex == 0)
//     {
//         pos = (-1.0f).xx;
//         output.Color = float4(1.0f, 0.0f, 0.0f, 1.0f);
//     }
//     else
//     {
//         if (input.VertexIndex == 1)
//         {
//             pos = float2(1.0f, -1.0f);
//             output.Color = float4(0.0f, 1.0f, 0.0f, 1.0f);
//         }
//         else
//         {
//             if (input.VertexIndex == 2)
//             {
//                 pos = float2(0.0f, 1.0f);
//                 output.Color = float4(0.0f, 0.0f, 1.0f, 1.0f);
//             }
//         }
//     }
//     output.Position = float4(pos, 0.0f, 1.0f);
//     return output;
// }
//
// [shader("fragment")]
// float4 FragmentMain(float4 Color : TEXCOORD0) : SV_Target0
// {
//     return Color;
// }
