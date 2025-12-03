#version 450 core
#include "../common/common.glsl"
#ifdef VERTEX_SHADER

void main()
{
	
	uint objectID       = gl_InstanceIndex;
    uint indexOffset    = gl_VertexIndex;

    mat4 model          = GetModelMatrix(objectID);
    uint index          = GetIndex(objectID, indexOffset);
    vec4 pos            = GetPosition(objectID, index);
    gl_Position = CAMERAINFO.data.viewProj * model * pos;

}
#endif

#ifdef FRAGMENT_SHADER
void main()
{
}
#endif
