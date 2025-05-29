#version 450
layout(column_major) uniform;
layout(column_major) buffer;

#line 26 0
layout(location = 0)
out vec4 entryPointParam_FragmentMain_0;


#line 1
layout(location = 0)
in vec3 data_color_0;


#line 26
void main()
{

#line 26
    entryPointParam_FragmentMain_0 = vec4(data_color_0, 1.0);

#line 26
    return;
}

