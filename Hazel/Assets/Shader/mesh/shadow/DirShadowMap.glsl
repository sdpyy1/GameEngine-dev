#version 450 core
#include "../../common/common.glsl"
#ifdef VERTEX_SHADER
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
 // ����Ҫ���ƣ�ֻ��Ҫ��Ȳ���ִ��

}
#endif
