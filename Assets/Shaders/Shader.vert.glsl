#version 450
layout(column_major) uniform;
layout(column_major) buffer;

#line 11395 0
struct _MatrixStorage_float4x4std140_0
{
    vec4  data_0[4];
};


#line 10 1
layout(binding = 0, set = 1)
layout(std140) uniform block_MatrixStorage_float4x4std140_0
{
    vec4  data_0[4];
}model_0;

#line 12
layout(binding = 1, set = 1)
layout(std140) uniform block_MatrixStorage_float4x4std140_1
{
    vec4  data_0[4];
}view_0;

#line 14
layout(binding = 2, set = 1)
layout(std140) uniform block_MatrixStorage_float4x4std140_2
{
    vec4  data_0[4];
}projection_0;

#line 14
mat4x4 unpackStorage_0(_MatrixStorage_float4x4std140_0 _S1)
{

#line 14
    return mat4x4(_S1.data_0[0][0], _S1.data_0[0][1], _S1.data_0[0][2], _S1.data_0[0][3], _S1.data_0[1][0], _S1.data_0[1][1], _S1.data_0[1][2], _S1.data_0[1][3], _S1.data_0[2][0], _S1.data_0[2][1], _S1.data_0[2][2], _S1.data_0[2][3], _S1.data_0[3][0], _S1.data_0[3][1], _S1.data_0[3][2], _S1.data_0[3][3]);
}

layout(location = 0)
in vec3 position_0;


#line 17
layout(location = 1)
in vec3 color_0;


#line 17
layout(location = 2)
in vec2 texCoord_0;


#line 17
layout(location = 0)
out vec3 data_color_0;


#line 17
layout(location = 1)
out vec2 data_texCoord_0;


#line 3
struct Vertex_0
{
    vec3 color_1;
    vec2 texCoord_1;
};


#line 17
void main()
{

#line 17
    Vertex_0 _S2;

    _S2.color_1 = color_0;
    _S2.texCoord_1 = texCoord_0;

#line 20
    _MatrixStorage_float4x4std140_0 _S3 = { model_0.data_0 };

#line 20
    _MatrixStorage_float4x4std140_0 _S4 = { view_0.data_0 };

#line 20
    _MatrixStorage_float4x4std140_0 _S5 = { projection_0.data_0 };

    vec4 _S6 = (((unpackStorage_0(_S5)) * ((((unpackStorage_0(_S4)) * ((((unpackStorage_0(_S3)) * (vec4(position_0, 1.0))))))))));

#line 22
    vec4 _S7 = _S6;
    _S7.y = - _S6.y;

#line 22
    gl_Position = _S7;

#line 22
    data_color_0 = _S2.color_1;

#line 22
    data_texCoord_0 = _S2.texCoord_1;

#line 22
    return;
}

