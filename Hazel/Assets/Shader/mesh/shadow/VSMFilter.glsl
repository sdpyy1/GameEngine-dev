#version 450 core
#include "../../common/common.glsl"
#include "../../common/constant.glsl"
#ifdef COMPUTE_SHADER

layout(set = 1, binding = 0) uniform texture2D IN_TEX[6];
layout(set = 1, binding = 1, rgba32f) uniform image2D OUT_TEX[6];
#define FILTER_SIZE 5
#define THREAD_SIZE_X 16
#define THREAD_SIZE_Y 16
#define THREAD_SIZE_Z 1
layout (local_size_x = THREAD_SIZE_X, 
		local_size_y = THREAD_SIZE_Y, 
		local_size_z = THREAD_SIZE_Z) in;
void main() 
{	
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    uint faceIndex = gl_GlobalInvocationID.z;

    ivec2 texSize = imageSize(OUT_TEX[faceIndex]);
    if(pixelCoord.x >= texSize.x || pixelCoord.y >= texSize.y){return;}
    vec4 colorSum = vec4(0.0);
    int sampleCount = 0;
    int halfFilter = FILTER_SIZE / 2;
    for(int y = -halfFilter; y <= halfFilter; y++)
        {
            for(int x = -halfFilter; x <= halfFilter; x++)
            {
                ivec2 sampleCoord = pixelCoord + ivec2(x, y);
                sampleCoord = clamp(sampleCoord, ivec2(0, 0), texSize - ivec2(1, 1));
            vec4 texColor = textureLod(sampler2D(IN_TEX[faceIndex], SAMPLER[0]), 
                                           vec2(sampleCoord) / vec2(texSize), 
                                           0.0);
                colorSum += texColor;
                sampleCount++;
            }
        }

    vec4 blurColor = colorSum / float(sampleCount);
    imageStore(OUT_TEX[faceIndex], pixelCoord, blurColor);
}
#endif