#version 450 core
#include "../common/common.glsl"
#include "../common/constant.glsl"
#include "../common/intersection.glsl"
#ifdef COMPUTE_SHADER
bool ProjectAABBToScreenUV(BoundingBox aabb, mat4 VP, vec2 screenSize, out vec4 uvAABB, out float boxZMin) {
    // AABB的8个顶点（世界空间）
    vec3 corners[8] = {
        {aabb.minBound.x, aabb.minBound.y, aabb.minBound.z},
        {aabb.maxBound.x, aabb.minBound.y, aabb.minBound.z},
        {aabb.minBound.x, aabb.maxBound.y, aabb.minBound.z},
        {aabb.maxBound.x, aabb.maxBound.y, aabb.minBound.z},
        {aabb.minBound.x, aabb.minBound.y, aabb.maxBound.z},
        {aabb.maxBound.x, aabb.minBound.y, aabb.maxBound.z},
        {aabb.minBound.x, aabb.maxBound.y, aabb.maxBound.z},
        {aabb.maxBound.x, aabb.maxBound.y, aabb.maxBound.z}
    };

    bool first = true;
    boxZMin = 1.0f;
    uvAABB = vec4(1.0f, 1.0f, 0.0f, 0.0f);

    for (int i = 0; i < 8; i++) {
        vec4 clipPos = VP * vec4(corners[i], 1.0f);
        if (clipPos.w <= 0.0001f) {
            continue;
        }
        vec3 ndc = clipPos.xyz / clipPos.w;
        float u = (ndc.x + 1.0f) * 0.5f;
        float v = (1.0f - ndc.y) * 0.5f;
        float depth = ndc.z;
        if (first) {
            uvAABB = vec4(u, v, u, v);
            boxZMin = depth;
            first = false;
        } else {
            uvAABB.x = min(uvAABB.x, u); // uMin
            uvAABB.y = min(uvAABB.y, v); // vMin
            uvAABB.z = max(uvAABB.z, u); // uMax
            uvAABB.w = max(uvAABB.w, v); // vMax
            boxZMin = min(boxZMin, depth); // 保留最靠近相机的深度
        }
    }
    uvAABB = clamp(uvAABB, 0.0f, 1.0f);
    if (first) {
        return false;
    }

    return true;
}
layout(set = 1, binding = 1)uniform texture2D HZB;

bool IsAABBOccludedByHZB(vec4 uvAABB, float boxZMin,  vec2 screenSize) {
    // 1. 计算UV包围盒
    ivec2 pixelMin = ivec2(uvAABB.x * screenSize.x, uvAABB.y * screenSize.y);
    ivec2 pixelMax = ivec2(uvAABB.z * screenSize.x, uvAABB.w * screenSize.y);
    pixelMin = clamp(pixelMin, ivec2(0), ivec2(screenSize) - 1);
    pixelMax = clamp(pixelMax, ivec2(0), ivec2(screenSize) - 1);

    // 2. 选择最优Mip层级
    ivec2 rectSize = pixelMax - pixelMin + 1;
    int mipLevel = 0;
    while (mipLevel < 10) { // 最多10级Mip
        ivec2 mipDim = textureSize(sampler2D(HZB,SAMPLER[0]), mipLevel);
        if (mipDim.x == 0 || mipDim.y == 0) break;
        if ((1 << mipLevel) >= rectSize.x && (1 << mipLevel) >= rectSize.y) {
            break;
        }
        mipLevel++;
    }

    // 3. 转换像素坐标到Mip层级的UV
    vec2 mipUVMin = vec2(pixelMin) / vec2(textureSize(sampler2D(HZB,SAMPLER[0]), 0));
    vec2 mipUVMax = vec2(pixelMax) / vec2(textureSize(sampler2D(HZB,SAMPLER[0]), 0));

    // 4. 采样该Mip层级的最大深度（使用textureGather或手动采样）
    vec2 mipSize = vec2(textureSize(sampler2D(HZB,SAMPLER[0]), mipLevel));
    vec2 uvStep = (mipUVMax - mipUVMin) / 2.0f;
    float depth1 = textureLod(sampler2D(HZB,SAMPLER[0]), mipUVMin, mipLevel).r;
    float depth2 = textureLod(sampler2D(HZB,SAMPLER[0]), vec2(mipUVMax.x, mipUVMin.y), mipLevel).r;
    float depth3 = textureLod(sampler2D(HZB,SAMPLER[0]), vec2(mipUVMin.x, mipUVMax.y), mipLevel).r;
    float depth4 = textureLod(sampler2D(HZB,SAMPLER[0]), mipUVMax, mipLevel).r;
    float hzbMaxDepth = max(max(depth1, depth2), max(depth3, depth4));
    const float depthBias = 0.0005f;
    return (hzbMaxDepth + depthBias) <= boxZMin;
}
const uint MESH_PASS_TYPE_BASE = 0u;
const uint MESH_PASS_TYPE_DIRECTIONLIGHT_SHADOW = 1u;
const uint MESH_PASS_TYPE_POINTLIGHT_SHADOW = 2u;

#include "../common/gizmo.glsl"

#define LOCAL_X 32   // instance
#define LOCAL_Y 1    // MeshPassType
#define LOCAL_Z 1
layout(set = 1, binding = 0) buffer drawbuffer{
    uint instanceCount;
    uint passType;
    uint index;  
    uint _padding;
    RHIIndirectCommand buffers[MAX_PER_FRAME_INSTANCE_SIZE];
} ALL_CULLING_BUFFERS[];


layout(local_size_x = LOCAL_X, local_size_y = LOCAL_Y, local_size_z = LOCAL_Z) in;
void main()
{ 
    uint threadInstanceId = gl_GlobalInvocationID.x;
    uint passTypeId = gl_GlobalInvocationID.y;
    if(threadInstanceId >= ALL_CULLING_BUFFERS[passTypeId].instanceCount){
        return;
    }
    uint instanceId = ALL_CULLING_BUFFERS[passTypeId].buffers[threadInstanceId].firstInstance;
    mat4 modelMatrix = GetModelMatrix(instanceId);
    BoundingBox aabb = GetOriginBoundingBox(instanceId);    
    aabb = BoundingBoxTransform(aabb,modelMatrix);

    // 摄像机剔除  TODO：用前一帧的HIZ进行遮挡剔除？
    if(ALL_CULLING_BUFFERS[passTypeId].passType == MESH_PASS_TYPE_BASE){

        Camera camera;
        if(GetRenderSetting().ClusterLightFrustum == 1){ 
            camera = GetDefaultCamera();
        }else{
            camera = GetCamera();
        }

        if(GetRenderSetting().renderBoundingBox == 1){
            AddGizmoBoundingBox(aabb, vec4(1,0,0,1));
        }

        bool isVisiable = FrustumIntersectBox(camera.frustum, aabb);

        if(!isVisiable){
            ALL_CULLING_BUFFERS[passTypeId].buffers[threadInstanceId].instanceCount = 0u;
        }else{
            // 视锥内进行遮挡剔除（如果整个AABB盒的深度都比HZB深度大，则进行遮挡剔除）
            // 1. 空间中的AABB投影到屏幕空间，得到最大最小4个顶点
            // TODO:遮挡剔除还不稳定
            // vec4 uvAABB;
            // float boxZMin;
            // ProjectAABBToScreenUV(aabb, camera.viewProj, vec2(camera.width,camera.height), uvAABB, boxZMin);
            // bool isOccluded = IsAABBOccludedByHZB(uvAABB, boxZMin, vec2(camera.width,camera.height));
            // if(isOccluded){
            //     ALL_CULLING_BUFFERS[passTypeId].buffers[threadInstanceId].instanceCount = 0u;
            // }
        }

    }else if(ALL_CULLING_BUFFERS[passTypeId].passType == MESH_PASS_TYPE_DIRECTIONLIGHT_SHADOW){  // CSM剔除
        DirectionLight light = GetDirectionLight();
        
        bool isVisiable = FrustumIntersectBox(light.frustum[ALL_CULLING_BUFFERS[passTypeId].index], aabb);  
        if(!isVisiable){
            ALL_CULLING_BUFFERS[passTypeId].buffers[threadInstanceId].instanceCount = 0u;
        }
    }else if(ALL_CULLING_BUFFERS[passTypeId].passType == MESH_PASS_TYPE_POINTLIGHT_SHADOW){ // 点光剔除
        if(ALL_CULLING_BUFFERS[passTypeId].index >= GetPointLightCount()){
            return;
        }
        BoundingSphere sphere = GetPointLight(ALL_CULLING_BUFFERS[passTypeId].index).sphere;

        bool isVisiable = SphereIntersectBox(sphere, aabb);
        if(!isVisiable){
            ALL_CULLING_BUFFERS[passTypeId].buffers[threadInstanceId].instanceCount = 0u;
        }

    }
}


#endif