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
    // 摄像机剔除
    if(ALL_CULLING_BUFFERS[passTypeId].passType == MESH_PASS_TYPE_BASE){

        Camera camera = GetCamera();
        BoundingBox aabb = GetOriginBoundingBox(instanceId);    

        aabb = BoundingBoxTransform(aabb,modelMatrix);
        if(GetRenderSetting().renderBoundingBox == 1){
            AddGizmoBoundingBox(aabb, vec4(1,0,0,1));
        }

        bool isVisiable = FrustumIntersectBox(camera.frustum, aabb);

        if(!isVisiable){
            ALL_CULLING_BUFFERS[passTypeId].buffers[threadInstanceId].instanceCount = 0u;
        }

    }else if(ALL_CULLING_BUFFERS[passTypeId].passType == MESH_PASS_TYPE_DIRECTIONLIGHT_SHADOW){ // 定向光剔除

    }else if(ALL_CULLING_BUFFERS[passTypeId].passType == MESH_PASS_TYPE_POINTLIGHT_SHADOW){ // 点光剔除

    }
}


#endif