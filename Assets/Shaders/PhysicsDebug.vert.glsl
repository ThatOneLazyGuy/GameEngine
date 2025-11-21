#version 450
layout(column_major) uniform;
layout(column_major) buffer;

#line 11395 0
struct _MatrixStorage_float4x4std140_0
{
    vec4  data_0[4];
};


#line 4 1
layout(binding = 0, set = 1)
layout(std140) uniform block_MatrixStorage_float4x4std140_0
{
    vec4  data_0[4];
}model_0;

#line 6
layout(binding = 1, set = 1)
layout(std140) uniform block_MatrixStorage_float4x4std140_1
{
    vec4  data_0[4];
}view_0;

#line 8
layout(binding = 2, set = 1)
layout(std140) uniform block_MatrixStorage_float4x4std140_2
{
    vec4  data_0[4];
}projection_0;

#line 8
mat4x4 unpackStorage_0(_MatrixStorage_float4x4std140_0 _S1)
{

#line 8
    return mat4x4(_S1.data_0[0][0], _S1.data_0[0][1], _S1.data_0[0][2], _S1.data_0[0][3], _S1.data_0[1][0], _S1.data_0[1][1], _S1.data_0[1][2], _S1.data_0[1][3], _S1.data_0[2][0], _S1.data_0[2][1], _S1.data_0[2][2], _S1.data_0[2][3], _S1.data_0[3][0], _S1.data_0[3][1], _S1.data_0[3][2], _S1.data_0[3][3]);
}

layout(location = 0)
in vec3 position_0;


#line 11
layout(location = 1)
in vec3 color_0;


#line 11
layout(location = 0)
out vec3 color_1;


#line 11
void main()
{

#line 11
    _MatrixStorage_float4x4std140_0 _S2 = { model_0.data_0 };

#line 11
    _MatrixStorage_float4x4std140_0 _S3 = { view_0.data_0 };

#line 11
    _MatrixStorage_float4x4std140_0 _S4 = { projection_0.data_0 };

    vec4 _S5 = (((unpackStorage_0(_S4)) * ((((unpackStorage_0(_S3)) * ((((unpackStorage_0(_S2)) * (vec4(position_0, 1.0))))))))));

#line 13
    vec4 _S6 = _S5;
    _S6.y = - _S5.y;

#line 13
    gl_Position = _S6;

#line 13
    color_1 = color_0;

#line 13
    return;
}

