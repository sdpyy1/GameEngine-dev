#version 450 core
#include "../../common/common.glsl"
#ifdef VERTEX_SHADER
layout(push_constant) uniform PushConstants
{
    uint cascadeIndex;
};

void main()
{ 
    DirectionLight light = GetDirectionLight();
    uint objectID       = gl_InstanceIndex;
    uint indexOffset    = gl_VertexIndex;
    mat4 model          = GetModel(objectID);
    uint index          = GetIndex(objectID, indexOffset);
    vec4 pos            = GetPos(objectID, index);
    vec2 texCoord       = GetTexCoord(objectID, index);    

    gl_Position = light.viewProj[cascadeIndex] * model * pos;
}

#endif

#ifdef FRAGMENT_SHADER
void main()
{
 // ����Ҫ���ƣ�ֻ��Ҫ��Ȳ���ִ��

}
#endif
