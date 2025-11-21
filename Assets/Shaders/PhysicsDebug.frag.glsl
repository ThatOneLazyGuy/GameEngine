#version 450
layout(column_major) uniform;
layout(column_major) buffer;

#line 17 0
layout(location = 0)
out vec4 entryPointParam_FragmentMain_0;


#line 17
layout(location = 0)
in vec3 color_0;


#line 17
void main()
{

#line 17
    entryPointParam_FragmentMain_0 = vec4(color_0, 1.0);

#line 17
    return;
}

