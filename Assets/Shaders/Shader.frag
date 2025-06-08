#version 450
layout(column_major) uniform;
layout(column_major) buffer;

#line 33 0
layout(binding = 0)
uniform sampler2D texture_diffuse0_0;


#line 957 1
layout(location = 0)
out vec4 entryPointParam_FragmentMain_0;


#line 957
layout(location = 1)
in vec2 data_texCoord_0;


#line 45 0
void main()
{

#line 45
    entryPointParam_FragmentMain_0 = (texture((texture_diffuse0_0), (data_texCoord_0)));

#line 45
    return;
}

