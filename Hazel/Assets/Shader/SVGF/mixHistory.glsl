#version 450 core
#include "../common/common.glsl"
#include "../common/math.glsl"
#ifdef COMPUTE_SHADER

layout(set = 1, binding = 0, rgba32f) uniform image2D DIRECT_COLOR;
layout(set = 1, binding = 1, rgba32f) uniform image2D IN_DIRECT_COLOR;
layout(set = 1, binding = 2, rgba32f) uniform image2D DIRECT_COLOR_HISTORY;
layout(set = 1, binding = 3, rgba32f) uniform image2D IN_DIRECT_COLOR_HISTORY;
layout(set = 1, binding = 4, rgba32f) uniform image2D DIRECT_VARIANCE;
layout(set = 1, binding = 5, rgba32f) uniform image2D IN_DIRECT_VARIANCE;
layout(set = 1, binding = 6, rgba32f) uniform image2D DIRECT_VARIANCE_HISTORY;
layout(set = 1, binding = 7, rgba32f) uniform image2D IN_DIRECT_VARIANCE_HISTORY;
layout(set = 1, binding = 8) uniform texture2D velocityTexture;
layout(set = 1, binding = 9, rgba32f) uniform image2D DIR_ORGIN_COLOR;
layout(set = 1, binding = 10, rgba32f) uniform image2D IN_DIR_ORGIN_COLOR;

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main() 
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    vec2 uv = ScreenPixToUV(pixel); 
    vec2 velocity   = texelFetch(velocityTexture,pixel,0).rg;
    vec2 prevUV = uv - velocity;
    ivec2 prevPixel = UVToNearestScreenPix(prevUV);
    prevPixel = clamp(prevPixel, ivec2(0), imageSize(DIRECT_COLOR_HISTORY) - 1);
    bool valid = (prevPixel.x >= 0.0 && prevPixel.x <= imageSize(DIRECT_COLOR_HISTORY).x) && 
            (prevPixel.y >= 0.0 && prevPixel.y <= imageSize(DIRECT_COLOR_HISTORY).y);
    // 直接光混合
    {
        vec3 prevDirectColor = imageLoad(DIRECT_COLOR_HISTORY,prevPixel).xyz;
        vec3 curDirectColor = imageLoad(DIR_ORGIN_COLOR,pixel).xyz;
        vec3 prevDirVariance = imageLoad(DIRECT_VARIANCE_HISTORY,prevPixel).xyz;
	    float Dirluminance 	= RGBtoLuminance(clamp(curDirectColor, vec3(0.0f), vec3(100.0f)));
	    Dirluminance = max(0.0f, Dirluminance);
        float dirHistoryLength = prevDirVariance.z;
	    float dirCurLength = min(dirHistoryLength + 1.0f, 100.0f); 
        float dirBlend = 0.05;
        if (any(isnan(prevDirectColor)) || !valid)  // 重新积累
        {
            dirHistoryLength = 0.0f;
            dirBlend = 1.0f;
        }
        if(dirHistoryLength != 0.0f) dirBlend = max(1.0 / 50.0, 1 / dirHistoryLength);	
	    vec2 DirMoments = vec2(Dirluminance, Dirluminance * Dirluminance);

        vec4 DirColorOut = vec4(mix(prevDirectColor,curDirectColor,dirBlend),DirMoments.y - DirMoments.x * DirMoments.x);
        vec4 DirMomentsOut = vec4(mix(prevDirVariance.xy,DirMoments,dirBlend),dirCurLength,0);
        imageStore(DIRECT_COLOR,pixel,DirColorOut);
        imageStore(DIRECT_VARIANCE,pixel,DirMomentsOut);

    }

    // 间接光混合
   { 
        vec3 prevInDirectColor = imageLoad(IN_DIRECT_COLOR_HISTORY,prevPixel).xyz;
        vec3 prevInDirVariance= imageLoad(IN_DIRECT_VARIANCE_HISTORY,prevPixel).xyz;
        vec3 curInDirectColor = imageLoad(IN_DIR_ORGIN_COLOR,pixel).xyz;
        float inDirluminance 	= RGBtoLuminance(clamp(curInDirectColor, vec3(0.0f), vec3(100.0f)));
        inDirluminance 			= max(0.0f, inDirluminance);
        vec2 InDirMoments       = vec2(inDirluminance, inDirluminance * inDirluminance);
        float IndirHistoryLength = prevInDirVariance.z;
        float IndirCurLength = min(IndirHistoryLength + 1.0f, 100.0f); 
        float indirBlend = 0.05;
        if (any(isnan(prevInDirectColor)) || !valid) 
        {
            IndirHistoryLength = 0.0f;
            indirBlend = 1.0f;
        }
        if(IndirHistoryLength != 0.0f) indirBlend = max(1.0 / 50.0, 1 / IndirHistoryLength);	
        vec4 InDirColorOut = vec4(mix(prevInDirectColor,curInDirectColor,indirBlend),InDirMoments.y - InDirMoments.x * InDirMoments.x);
        vec4 InDirMomentsOut = vec4(mix(prevInDirVariance.xy,InDirMoments,indirBlend),IndirCurLength,0);
        imageStore(IN_DIRECT_COLOR,pixel,InDirColorOut);

        imageStore(IN_DIRECT_VARIANCE,pixel,InDirMomentsOut);
   }
}

#endif
