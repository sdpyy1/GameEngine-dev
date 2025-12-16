#version 450 core
#include "../common/common.glsl"
#include "../common/constant.glsl"
#ifdef COMPUTE_SHADER
#define LOCAL_X 16
#define LOCAL_Y 1
#define LOCAL_Z 1
layout(set=1,binding=0) buffer drawbuffer{
    uint instanceCount;
    uint _padding[3];
    RHIIndirectCommand buffers[MAX_PER_FRAME_INSTANCE_SIZE];
} ALL_CULLING_BUFFERS[10];
layout(local_size_x = LOCAL_X, local_size_y = LOCAL_Y, local_size_z = LOCAL_Z) in;
void main()
{ 
    uint threadInstanceId = gl_GlobalInvocationID.x;
    uint passId = gl_GlobalInvocationID.y;
    if(threadInstanceId >= ALL_CULLING_BUFFERS[passId].instanceCount){
        return;
    }

    // instanceId可以拿到这个实例的所有信息
    uint instanceId = ALL_CULLING_BUFFERS[passId].buffers[threadInstanceId].firstInstance;




    ALL_CULLING_BUFFERS[passId].buffers[threadInstanceId].instanceCount = 0u;
}


#endif