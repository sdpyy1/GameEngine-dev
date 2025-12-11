#version 450 core
#include "../common/common.glsl"
#include "../common/DDGI.glsl"
#include "../common/gizmo.glsl"
/*
每个线程处理一个探针的8*8区域的一个像素
*/
layout(set = 1, rgba32f, binding = 0) uniform image2DArray o_Texture;
layout(set = 1, rgba32f, binding = 1) uniform image2DArray  IN_RayData;
#ifdef COMPUTE_SHADER
layout(local_size_x = DDGI_PROBE_NUM_TEXELS_IRRANDIANCE, local_size_y = DDGI_PROBE_NUM_TEXELS_IRRANDIANCE, local_size_z = 1) in;
void main(){
	DDGISetting volume = GetDDGISetting();
    uvec3 groupID = gl_WorkGroupID;  // 对于dispatch的ID
    uvec3 invocationID = gl_GlobalInvocationID; // 相对于全局的调用ID
    uvec3 LocalInvocationID = gl_LocalInvocationID; // 相对于组内的调用ID

    // 判断是不是边界（边界是填充，不是计算），利用的是在本组内的索引xy，因为这个索引对应的是一个探针的8*8数据块，所以如果索引是0或者7，那么就是边界
    bool isBorderTexel = (LocalInvocationID.x == 0 || LocalInvocationID.x == (DDGI_PROBE_NUM_TEXELS_IRRANDIANCE_INTERIOR + 1)) || (LocalInvocationID.y == 0 || LocalInvocationID.y == (DDGI_PROBE_NUM_TEXELS_IRRANDIANCE_INTERIOR + 1));

    // 从invocationID获取当先要处理的探针索引
    uint probeIndex = DDGIGetProbeIndex(invocationID, DDGI_PROBE_NUM_TEXELS_IRRANDIANCE, volume);
    //uint probeIndex = (groupID.z * DDGIGetProbesPerPlane(volume.probeCount)) + groupID.y * volume.probeCount.x + groupID.x;

    // Early out: no probe maps to this thread
    uint numProbes = (volume.probeCount.x * volume.probeCount.y * volume.probeCount.z);
    if (probeIndex >= numProbes || probeIndex < 0) return;

    vec4 result = vec4(0.0);

    // 不是边界，就需要计算了 
    if(!isBorderTexel){
        uvec3 threadCoords = uvec3(groupID.x * DDGI_PROBE_NUM_TEXELS_IRRANDIANCE_INTERIOR, groupID.y * DDGI_PROBE_NUM_TEXELS_IRRANDIANCE_INTERIOR, invocationID.z) + LocalInvocationID - uvec3(1, 1, 0);
        vec2 probeOctantUV = DDGIGetNormalizedOctahedralCoordinates(uvec2(threadCoords.xy), DDGI_PROBE_NUM_TEXELS_IRRANDIANCE_INTERIOR);
        vec3 probeRayDirection = DDGIGetOctahedralDirection(probeOctantUV);

        // 遍历当前探针的所有光线
        for (uint rayIndex = 0; rayIndex < volume.raysPerProbe; rayIndex++){
            // 获取ray的方向
            vec3 rayDirection = normalize(RTXGISphericalFibonacci(rayIndex,volume.raysPerProbe));   // TODO: 优化点，因为每个探针计算这个都是一模一样的
            // 当前要存储的方向与这跟光线的cos，两个方向夹角越小，权重越大
            float weight = max(0.0, dot(probeRayDirection, rayDirection));
            // 解析RayData中的信息
            uvec3 rayDataTexCoords = DDGIGetRayDataTexelCoords(rayIndex, probeIndex, volume);  
            vec4 storedData = imageLoad(IN_RayData, ivec3(rayDataTexCoords));
            vec3 probeRayRadiance = storedData.xyz;
            float probeRayDistance = storedData.w;
            if(probeRayDistance < 0){  // 击中的是背面
                continue;
            }
            // 展示探针数据（射线长度和射线采集的Radiance）// TODO:后期可拓展为选中某个探针，可视化它的数据
            // if(volume.visulaize == 1 && probeIndex == 164 && LocalInvocationID == uvec3(1,1,0)){
            //     if(probeRayDistance < 1e27f){ 
            //         uvec3 prebeCoords = DDGIGetProbeCoords(probeIndex,volume);
            //         vec3 probePosition = DDGIGetProbeWorldPosition(prebeCoords,volume);
            //         AddGizmoLine(probePosition,probePosition + (rayDirection * probeRayDistance), vec4(probeRayRadiance*3,1));
            //     }
            // }
            result += vec4(probeRayRadiance * weight, weight);
        }

        // 工程化修正问题：需要结合蒙特卡洛积分理解
        float epsilon = float(volume.raysPerProbe);
        epsilon *= 1e-9f;
        result.rgb *= 1.f / (2.f * max(result.a, epsilon));

        // 时域加权混合
        vec4 history = imageLoad(o_Texture,ivec3(gl_GlobalInvocationID));
        vec3 delta = (result.rgb - history.rgb);

        float  hysteresis = 0.97; // TODO：混合系数是参数
        if (dot(history, history) == 0) hysteresis = 0.f;
        float probeIrradianceEncodingGamma = 5.0f;  // TODO：编码Gamma是参数
        
        result.rgb = pow(result.rgb, vec3(1.f / probeIrradianceEncodingGamma));
        float probeIrradianceThreshold = 0.25f; // TODO：阈值是参数
        if (RTXGIMaxComponent(history.rgb - result.rgb) > probeIrradianceThreshold)
        {
            // Lower the hysteresis when a large lighting change is detected
            hysteresis = max(0.f, hysteresis - 0.75f);
        }
        float probeBrightnessThreshold  = 0.10f; // TODO：阈值是参数
        if (RGBtoLuminance(delta) > probeBrightnessThreshold)
        {
            // Clamp the maximum per-update change in irradiance when a large brightness change is detected
            delta *= 0.25f;
        }
        const float c_threshold = 1.f / 1024.f;
        vec3 lerpDelta = (1.f - hysteresis) * delta;

        if (RTXGIMaxComponent(result.rgb) < RTXGIMaxComponent(history.rgb))
        {
            lerpDelta = min(max(vec3(c_threshold), abs(lerpDelta)), abs(delta)) * sign(lerpDelta);
        }
        result = vec4(history.rgb + lerpDelta, 1.f);

        // 可视化探针颜色
        if(volume.visulaize == 1 && LocalInvocationID == uvec3(1,1,0)){
            uvec3 prebeCoords = DDGIGetProbeCoords(probeIndex, volume);
            vec3 probePosition = DDGIGetProbeWorldPosition(prebeCoords, volume);
            AddGizmoSphere(probePosition,0.5,result);
        }

        imageStore(o_Texture, ivec3(gl_GlobalInvocationID), result);
    }else{
        // 边界
        memoryBarrier();        // 所有全局内存类型的屏障
        memoryBarrierShared();  // 对 shared 内存屏障
        barrier();   //这里有一个同步点 

        bool isCornerTexel = (LocalInvocationID.x == 0 || LocalInvocationID.x == (DDGI_PROBE_NUM_TEXELS_IRRANDIANCE - 1)) && (LocalInvocationID.y == 0 || LocalInvocationID.y == (DDGI_PROBE_NUM_TEXELS_IRRANDIANCE - 1));
        bool isRowTexel = (LocalInvocationID.x > 0 && LocalInvocationID.x < (DDGI_PROBE_NUM_TEXELS_IRRANDIANCE - 1));

        uvec3 copyCoordinates = uvec3(groupID.x * DDGI_PROBE_NUM_TEXELS_IRRANDIANCE, groupID.y * DDGI_PROBE_NUM_TEXELS_IRRANDIANCE, invocationID.z);

        if(isCornerTexel)
        {
            copyCoordinates.x += LocalInvocationID.x > 0 ? 1 : DDGI_PROBE_NUM_TEXELS_IRRANDIANCE_INTERIOR;
            copyCoordinates.y += LocalInvocationID.y > 0 ? 1 : DDGI_PROBE_NUM_TEXELS_IRRANDIANCE_INTERIOR;
        }
        else if(isRowTexel)
        {
            copyCoordinates.x += (DDGI_PROBE_NUM_TEXELS_IRRANDIANCE - 1) - LocalInvocationID.x;
            copyCoordinates.y += LocalInvocationID.y + ((LocalInvocationID.y > 0) ? -1 : 1);
        }
        else // Column Texel
        {
            copyCoordinates.x += LocalInvocationID.x + ((LocalInvocationID.x > 0) ? -1 : 1);
            copyCoordinates.y += (DDGI_PROBE_NUM_TEXELS_IRRANDIANCE - 1) - LocalInvocationID.y;
        }

        imageStore(o_Texture, ivec3(gl_GlobalInvocationID), imageLoad(o_Texture, ivec3(copyCoordinates)));
    }
}

#endif