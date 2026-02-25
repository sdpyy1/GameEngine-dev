#version 450 core
#include "../common/common.glsl"
#ifdef COMPUTE_SHADER

layout(set = 1, binding = 0, r32f) uniform image2D o_Texture;       // 输出
layout(set = 1, binding = 1, r32f) uniform readonly image2D i_Texture; // 输入只读

float GetMinDepth4x4(ivec2 pixelLocation){
    ivec2 BasePos = pixelLocation * 2;
    float A = imageLoad(i_Texture, BasePos).r;
    float B = imageLoad(i_Texture, BasePos + ivec2(1,0)).r;
    float C = imageLoad(i_Texture, BasePos + ivec2(0,1)).r;
    float D = imageLoad(i_Texture, BasePos + ivec2(1,1)).r;

    return max(max(A,B), max(C,D));
}

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
    ivec2 invocID = ivec2(gl_GlobalInvocationID.xy);

    if(invocID.x >= imageSize(o_Texture).x || invocID.y >= imageSize(o_Texture).y)
        return;

    float depth = GetMinDepth4x4(invocID);
    imageStore(o_Texture, invocID, vec4(depth, 0.0, 0.0, 1.0));
}

#endif