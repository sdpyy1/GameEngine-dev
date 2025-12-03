#version 450 core
#include "../common/common.glsl"
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
    uint objectID       = gl_InstanceIndex;
    uint indexOffset    = gl_VertexIndex;

    mat4 model          = GetModel(objectID);
    // mat4 prevModel      = GetPrevModel(objectID);
    uint index          = GetIndex(objectID, indexOffset);
    vec4 pos            = GetPos(objectID, index);
    vec3 worldNormal    = GetWorldNormal(GetNormal(objectID, index), model);
    vec4 worldTangent   = GetWorldTangent(GetTangent(objectID, index), model);
    vec3 color          = GetColor(objectID, index); 
    vec2 texCoord       = GetTexCoord(objectID, index);      

    OUT_POSITION        = model * pos;
    // OUT_PREV_POSITION   = prevModel * pos;
    OUT_COLOR           = color;
    OUT_TEXCOORD        = texCoord;
    OUT_NORMAL          = worldNormal;
    OUT_TANGENT         = worldTangent;
    OUT_ID              = objectID;

    gl_Position = CAMERAINFO.data.viewProj * model * pos;
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
layout (location = 0) out vec4 o_Position;
layout (location = 1) out vec4 o_Normal;
layout (location = 2) out vec4 o_MaterialInfo;
layout (location = 3) out vec4 o_Albedo;
void main()
{
    MaterialInfo MaterialInfo   = GetMaterialInfo(IN_ID);
    vec4 color          = vec4(IN_COLOR, 1.0f);
    vec4 diffuse        = GetDiffuse(MaterialInfo, IN_TEXCOORD);
    diffuse.a           = 1.0f;
    vec4 emission       = GetEmission(MaterialInfo,IN_TEXCOORD);
    vec3 normal         = GetNormal(MaterialInfo, IN_TEXCOORD, IN_NORMAL, IN_TANGENT);
    float roughness     = GetRoughness(MaterialInfo, IN_TEXCOORD);
    float metallic      = GetMetallic(MaterialInfo, IN_TEXCOORD);


    o_Position    = vec4(IN_POSITION);
    o_Normal    = vec4(normal,1);
    o_MaterialInfo    = vec4(roughness,metallic,1,1); 
    o_Albedo    = vec4(diffuse + emission);
}
#endif
