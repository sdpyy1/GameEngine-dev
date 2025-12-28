#version 450 core
#include "../common/common.glsl"
#ifdef COMPUTE_SHADER

layout(set = 1, binding = 0, rgba32f) uniform image2D OUTCOLOR;
layout(set = 1, binding = 1, rgba32f) uniform image2D directCOLOR;
layout(set = 1, binding = 2, rgba32f) uniform image2D inDirectCOLOR;
layout(set = 1, binding = 3, rgba32f) uniform image2D albedo;

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

void main() 
{
    
}

#endif
