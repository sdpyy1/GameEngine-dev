#version 450 core
#include "../common/common.glsl"
#include "../common/constant.glsl"
#include "../common/intersection.glsl"
#ifdef COMPUTE_SHADER

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
    uint _padding[2];
    RHIIndirectCommand buffers[MAX_PER_FRAME_INSTANCE_SIZE];
} ALL_CULLING_BUFFERS[4];
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
        }

    }else if(ALL_CULLING_BUFFERS[passTypeId].passType == MESH_PASS_TYPE_DIRECTIONLIGHT_SHADOW){ 
        DirectionLight light = GetDirectionLight();
        
        bool isVisiable = FrustumIntersectBox(light.frustum[0], aabb);   // TODO: 现在只考虑一个级联的剔除（导致现在只有CSM=0距离内有阴影），也就是后三个级联的剔除用的第一个视锥，这是错的！！！解决需要想办法把当前是处理第几个级联的信息传递进来~现在架构不够灵活，还不太好传递呢~
        if(!isVisiable){
            ALL_CULLING_BUFFERS[passTypeId].buffers[threadInstanceId].instanceCount = 0u;
        }


    }else if(ALL_CULLING_BUFFERS[passTypeId].passType == MESH_PASS_TYPE_POINTLIGHT_SHADOW){ // 点光剔除

        BoundingSphere sphere = GetPointLight(0).sphere;

        bool isVisiable = SphereIntersectBox(sphere, aabb);
        if(!isVisiable){
            ALL_CULLING_BUFFERS[passTypeId].buffers[threadInstanceId].instanceCount = 0u;
        }

    }
}


#endif