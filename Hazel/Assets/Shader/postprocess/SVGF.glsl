#version 450 core
#include "../common/common.glsl"
#include "../common/math.glsl"
#include "../common/SVGF.glsl"
#ifdef COMPUTE_SHADER
layout(set = 1, binding = 0, rgba32f) uniform image2D OUT_COLOR;
layout(set = 1, binding = 1, rgba32f) uniform image2D IN_COLOR;
layout(set = 1,binding = 2, rgba32f) uniform image2D velocityTexture;
layout(set = 1,binding = 3, rgba32f) uniform image2D history;
layout(set = 1,binding = 4, rgba32f) uniform readonly image2D POSITION;
layout(set = 1,binding = 5, rgba32f) uniform readonly image2D NORMAL;
layout(set = 1,binding = 6, rgba32f) uniform readonly image2D MATERIAL;
layout(set = 1,binding = 7, rgba32f) uniform readonly image2D ALBEDO;

layout(push_constant) uniform Uniforms
{
    uint curPassIndex; // 当前 Atrous pass 索引
};

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

void main(){
    ivec2 invocID = ivec2(gl_GlobalInvocationID.xy);
    ivec2 imgSize = ivec2(imageSize(OUT_COLOR));
    if (invocID.x >= imgSize.x || invocID.y >= imgSize.y) 
        return;

    vec3 colorSum = vec3(0.0);
    float weightSum = 0.0;
    vec3 normal = imageLoad(NORMAL, invocID).xyz;

    // 5x5 Atrous 核心 做了空洞滤波，相当于64*64的卷积核
    uint curStep = 1 << curPassIndex;
    for (int dy = -2; dy <= 2; ++dy) {
        for (int dx = -2; dx <= 2; ++dx) {
            ivec2 samplePos = invocID + ivec2(dx, dy) * int(curStep);

            // 边界检查
            samplePos = clamp(samplePos, ivec2(0), imgSize - 1);

            vec3 sampleColor = imageLoad(IN_COLOR, samplePos).xyz;
            // 高斯
            float gaussWeight = gaussKernel5x5[dx + 2 + (dy + 2) * 5];
            // 法线
            vec3 sampleNormal = imageLoad(NORMAL, samplePos).xyz;
            float normalWeight = pow(max(dot(normal, sampleNormal), 0.0), 128.0);

            float weight = gaussWeight*normalWeight;

            colorSum += sampleColor * weight;
            weightSum += weight;
        }
    }

    vec3 result = colorSum / max(1e-4,weightSum);
    imageStore(OUT_COLOR, invocID, vec4(result, 1.0));
}
#endif
