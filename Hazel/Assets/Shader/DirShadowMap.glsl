#version 450 core
#include "common/common.glsl"
#ifdef VERTEX_SHADER
// 传入content表示当前渲染第几个级联
layout(push_constant) uniform PushConstants
{
    uint cascadeIndex;
};

void main()
{ 
    DirLightInfo light = FetchDirLightInfo();
    uint objectID       = gl_InstanceIndex;
    uint indexOffset    = gl_VertexIndex;
    mat4 model          = FetchModel(objectID);
    uint index          = FetchIndex(objectID, indexOffset);
    vec4 pos            = FetchPos(objectID, index);
    vec2 texCoord       = FetchTexCoord(objectID, index);    

    gl_Position = light.viewProj[cascadeIndex] * model * pos;
}

#endif

#ifdef FRAGMENT_SHADER
void main()
{
 // 不需要绘制，只需要深度测试执行

}
#endif
