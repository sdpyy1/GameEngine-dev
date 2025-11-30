#version 450 core
#ifdef VERTEX_SHADER
layout(location = 0) out uint OUT_ID;

void main() 
{
    // 唯一目的就是输出当前是渲染第几个billboard
    uint objectID  = gl_InstanceIndex;
    
    OUT_ID = objectID;
}
#endif

#ifdef GEOMETRY_SHADER
#include "common/common.glsl"
layout(points) in;
layout(triangle_strip, max_vertices = 4) out;   // 最多输出4个顶点，这四个顶点组成一个triangle_strip

layout(location = 0) in uint IN_ID[];

layout(location = 0) out uint OUT_TEXTURE_ID;
layout(location = 1) out vec2 OUT_UV;
layout(location = 2) out vec4 OUT_COLOR;

// textureId是icon的id，color是光的颜色
void Emit(vec3 center, vec2 extent, uint textureID, vec4 color){

    // 摄像机视角下的billboard的中心坐标
    vec4 viewCenter = u_CameraData.data.view * vec4(center, 1.0f);

    // 发射4个顶点，表示一个billboard的四个角，因为四个角的计算就是在View空间下，所以它会始终朝着摄像机
    gl_Position = u_CameraData.data.proj * (viewCenter + vec4(-extent.x, extent.y, 0.0f, 0.0f));
    OUT_TEXTURE_ID = textureID;
    OUT_UV = vec2(0.0f, 0.0f);
    OUT_COLOR = color;
	EmitVertex();

	gl_Position = u_CameraData.data.proj * (viewCenter + vec4(extent.x, extent.y, 0.0f, 0.0f));
	OUT_TEXTURE_ID = textureID;
    OUT_UV = vec2(1.0f, 0.0f);
    OUT_COLOR = color;
	EmitVertex();

    gl_Position = u_CameraData.data.proj * (viewCenter + vec4(-extent.x, -extent.y, 0.0f, 0.0f));
	OUT_TEXTURE_ID = textureID;
    OUT_UV = vec2(0.0f, 1.0f);
    OUT_COLOR = color;
	EmitVertex();

    gl_Position = u_CameraData.data.proj * (viewCenter + vec4(extent.x, -extent.y, 0.0f, 0.0f));
	OUT_TEXTURE_ID = textureID;
    OUT_UV = vec2(1.0f, 1.0f);
    OUT_COLOR = color;
	EmitVertex();

	EndPrimitive();
}
void main()
{
    uint objectID = IN_ID[0];
    GizmoBillboardInfo info = GIZMO_DRAW_DATA.worldBillboards[objectID];   // 获取当前billboard的信息
    Emit(info.center, info.extent, info.textureID, info.color);
}
#endif

#ifdef FRAGMENT_SHADER
#include "common/common.glsl"

layout(location = 0) in flat uint IN_TEXTURE_ID;
layout(location = 1) in vec2 IN_UV;
layout(location = 2) in vec4 IN_COLOR;
layout (location = 0) out vec4 OUT_COLOR;

void main()
{
    OUT_COLOR = IN_COLOR;
    if(IN_TEXTURE_ID != 0) OUT_COLOR *= pow(FetchTex2D(IN_TEXTURE_ID, IN_UV, 0), vec4(1.0/2.2));         

    if(OUT_COLOR.w < 0.00001f) discard;
}

#endif

