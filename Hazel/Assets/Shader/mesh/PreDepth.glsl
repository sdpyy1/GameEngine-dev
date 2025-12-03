#version 450 core
#include "../common/common.glsl"
#ifdef VERTEX_SHADER

void main()
{
	
	uint objectID       = gl_InstanceIndex;
    uint indexOffset    = gl_VertexIndex;

    mat4 model          = GetModel(objectID);
    uint index          = GetIndex(objectID, indexOffset);
    vec4 pos            = GetPos(objectID, index);
    gl_Position = u_CameraData.data.viewProj * model * pos;

}
#endif

#ifdef FRAGMENT_SHADER
void main()
{
}
#endif
