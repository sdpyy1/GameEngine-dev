#version 450 core
#include "common/common.glsl"
#ifdef VERTEX_SHADER
layout(location = 0) out vec4 OUT_POSITION;
// layout(location = 1) out vec4 OUT_PREV_POSITION;
layout(location = 2) out vec3 OUT_COLOR;
layout(location = 3) out vec2 OUT_TEXCOORD;
layout(location = 4) out vec3 OUT_NORMAL;
layout(location = 5) out vec4 OUT_TANGENT;
layout(location = 6) out flat uint OUT_ID;

void main() 
{
    uint objectID       = gl_InstanceIndex;   // ����ʱ�����õ�startʵ������ʵ��ID
    uint indexOffset    = gl_VertexIndex;

    mat4 model          = FetchModel(objectID);
    // mat4 prevModel      = FetchPrevModel(objectID);
    uint index          = FetchIndex(objectID, indexOffset);
    vec4 pos            = FetchPos(objectID, index);
    vec3 worldNormal    = FetchWorldNormal(FetchNormal(objectID, index), model);
    vec4 worldTangent   = FetchWorldTangent(FetchTangent(objectID, index), model);
    vec3 color          = FetchColor(objectID, index); 
    vec2 texCoord       = FetchTexCoord(objectID, index);      

    OUT_POSITION        = model * pos;
    // OUT_PREV_POSITION   = prevModel * pos;
    OUT_COLOR           = color;
    OUT_TEXCOORD        = texCoord;
    OUT_NORMAL          = worldNormal;
    OUT_TANGENT         = worldTangent;
    OUT_ID              = objectID;

    gl_Position = u_CameraData.data.viewProj * model * pos;
}

#endif

#ifdef FRAGMENT_SHADER
layout(location = 0) in vec4 IN_POSITION;
// layout(location = 1) in vec4 OUT_PREV_POSITION;
layout(location = 2) in vec3 IN_COLOR;
layout(location = 3) in vec2 IN_TEXCOORD;
layout(location = 4) in vec3 IN_NORMAL;
layout(location = 5) in vec4 IN_TANGENT;
layout(location = 6) in flat uint IN_ID;
layout (location = 0) out vec4 G_BUFFER_DIFFUSE_ROUGHNESS;
layout (location = 1) out vec4 G_BUFFER_NORMAL_METALLIC;
void main()
{
    Material material   = FetchMaterial(IN_ID);
    // vec4 color          = vec4(IN_COLOR, 1.0f);
    vec4 diffuse        = FetchDiffuse(material, IN_TEXCOORD);
    diffuse.a           = 1.0f;
    // vec3 emission       = FetchEmission(material);
    // vec3 normal         = FetchNormal(material, IN_TEXCOORD, IN_NORMAL, IN_TANGENT);
    // float roughness     = FetchRoughness(material, IN_TEXCOORD);
    // float metallic      = FetchMetallic(material, IN_TEXCOORD);
    G_BUFFER_DIFFUSE_ROUGHNESS    = vec4(diffuse);
    G_BUFFER_NORMAL_METALLIC           = vec4(1,1,1,1);
}
#endif
