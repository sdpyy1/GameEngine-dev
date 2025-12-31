#version 450 core
#include "../../common/common.glsl"
layout(push_constant) uniform point_light_setting {
    uint lightID;
}push_LIGHTID;
#ifdef VERTEX_SHADER
layout(location = 0) out vec4 OUT_POS;
layout(location = 1) out vec2 OUT_TEXCOORD;
layout(location = 2) out uint OUT_ID;
void main()
{
    uint objectID       = gl_InstanceIndex;
    uint indexOffset    = gl_VertexIndex;
    mat4 model          = GetModelMatrix(objectID);
    uint index          = GetIndex(objectID, indexOffset);
    vec4 pos            = GetPosition(objectID, index);
    vec2 texCoord       = GetTexCoord(objectID, index);

    OUT_POS             = model * pos;
    OUT_TEXCOORD        = texCoord;
    OUT_ID              = objectID;
}
#endif

#ifdef GEOMETRY_SHADER

layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

layout(location = 0) in vec4 IN_POS[];
layout(location = 1) in vec2 IN_TEXCOORD[];
layout(location = 2) in uint IN_ID[];
layout(location = 0) out vec4 OUT_POS;
layout(location = 1) out int OUT_INDEX;
layout(location = 2) out vec2 OUT_TEXCOORD;
layout(location = 3) out uint OUT_ID;

void Emit(  in int index, 
            in PointLight light, 
            in vec4 pos[3], 
            in vec2 coord[3], 
            in uint id[3])
{
    gl_Layer = index;

    gl_Position = light.proj * light.view[index] * pos[0];
    OUT_POS         = pos[0];
    OUT_INDEX       = index;
    OUT_TEXCOORD    = coord[0];
    OUT_ID          = id[0];
    EmitVertex();

    gl_Position = light.proj * light.view[index] * pos[1];
    OUT_POS         = pos[1];
    OUT_INDEX       = index;
    OUT_TEXCOORD    = coord[1];
    OUT_ID          = id[1];
    EmitVertex();

    gl_Position = light.proj * light.view[index] * pos[2];
    OUT_POS         = pos[2];
    OUT_INDEX       = index;
    OUT_TEXCOORD    = coord[2];
    OUT_ID          = id[2];
    EmitVertex();

    EndPrimitive();
}

void main()
{
    PointLight light = GetPointLight(push_LIGHTID.lightID);

    Emit(0, light, IN_POS, IN_TEXCOORD, IN_ID);
    Emit(1, light, IN_POS, IN_TEXCOORD, IN_ID);
    Emit(2, light, IN_POS, IN_TEXCOORD, IN_ID);
    Emit(3, light, IN_POS, IN_TEXCOORD, IN_ID);
    Emit(4, light, IN_POS, IN_TEXCOORD, IN_ID);
    Emit(5, light, IN_POS, IN_TEXCOORD, IN_ID);
}

#endif
#ifdef FRAGMENT_SHADER
layout(location = 0) in vec4 IN_POS;
layout(location = 1) in flat int IN_INDEX;
layout(location = 2) in vec2 IN_TEXCOORD;
layout(location = 3) in flat uint IN_ID;

layout(location = 0) out vec4 OUT_COLOR;

void main()
{
    PointLight light = GetPointLight(push_LIGHTID.lightID);
    float depth = length(IN_POS.xyz - light.position) / light.sphere.radius;
    OUT_COLOR = vec4(depth, 0.0, 0.0, 1.0);

}



#endif
