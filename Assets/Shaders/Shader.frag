#version 450
layout(column_major) uniform;
layout(column_major) buffer;

#line 44 0
layout(location = 0)
out vec4 entryPointParam_FragmentMain_0;


#line 1
layout(location = 1)
in vec2 data_texCoord_0;


#line 44
void main()
{

#line 44
    entryPointParam_FragmentMain_0 = vec4(data_texCoord_0, 0.0, 1.0);

#line 44
    return;
}

