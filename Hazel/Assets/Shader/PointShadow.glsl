#version 450 core
#include "common/common.glsl"
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
    mat4 model          = FetchModel(objectID);
    uint index          = FetchIndex(objectID, indexOffset);
    vec4 pos            = FetchPos(objectID, index);
    vec2 texCoord       = FetchTexCoord(objectID, index);

    OUT_POS             = model * pos;
    OUT_TEXCOORD        = texCoord;
    OUT_ID              = objectID;
}
#endif

#ifdef GEOMETRY_SHADER

layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;   // 模型的每个三角形都投射到6个面上

layout(location = 0) in vec4 IN_POS[];
layout(location = 1) in vec2 IN_TEXCOORD[];
layout(location = 2) in uint IN_ID[];
layout(location = 0) out vec4 OUT_POS;
layout(location = 1) out int OUT_INDEX;
layout(location = 2) out vec2 OUT_TEXCOORD;
layout(location = 3) out uint OUT_ID;

// 就是每个顶点都使用不同的光源View，相当于把点光源当作摄像机，朝着6个面都拍摄一张图片
void Emit(  in int index, 
            in PointLightInfo light, 
            in vec4 pos[3], 
            in vec2 coord[3], 
            in uint id[3])
{
    gl_Layer = index;  // 多层图片一起渲染

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
    PointLightInfo light = FetchPointLightInfo(push_LIGHTID.lightID);

    Emit(0, light, IN_POS, IN_TEXCOORD, IN_ID);
    Emit(1, light, IN_POS, IN_TEXCOORD, IN_ID);
    Emit(2, light, IN_POS, IN_TEXCOORD, IN_ID);
    Emit(3, light, IN_POS, IN_TEXCOORD, IN_ID);
    Emit(4, light, IN_POS, IN_TEXCOORD, IN_ID);
    Emit(5, light, IN_POS, IN_TEXCOORD, IN_ID);
}

#endif
#ifdef FRAGMENT_SHADER
// 注意：flat表示不需要插值，用于传递ID
layout(location = 0) in vec4 IN_POS;
layout(location = 1) in flat int IN_INDEX;
layout(location = 2) in vec2 IN_TEXCOORD;
layout(location = 3) in flat uint IN_ID;

layout(location = 0) out vec4 OUT_COLOR;

void main()
{
    PointLightInfo light = FetchPointLightInfo(push_LIGHTID.lightID);
    float depth = length(IN_POS.xyz - light.position) / light.sphere.radius;  // 模型到点光源距离，压缩到0-1，大于1表示模型在点光源外
    OUT_COLOR = vec4(depth, 0.0, 0.0, 1.0); //距离作为深度

}



#endif
