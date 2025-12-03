#version 450 core
#ifdef VERTEX_SHADER
layout(location = 0) out uint OUT_ID;

void main() 
{
    uint objectID  = gl_InstanceIndex;
    
    OUT_ID = objectID;
}
#endif

#ifdef GEOMETRY_SHADER
#include "../common/common.glsl"
layout(points) in;
layout(triangle_strip, max_vertices = 4) out; 

layout(location = 0) in uint IN_ID[];

layout(location = 0) out uint OUT_TEXTURE_ID;
layout(location = 1) out vec2 OUT_UV;
layout(location = 2) out vec4 OUT_COLOR;

void Emit(vec3 center, vec2 extent, uint textureID, vec4 color){
    vec4 viewCenter = CAMERAINFO.data.view * vec4(center, 1.0f);
    gl_Position = CAMERAINFO.data.proj * (viewCenter + vec4(-extent.x, extent.y, 0.0f, 0.0f));
    OUT_TEXTURE_ID = textureID;
    OUT_UV = vec2(0.0f, 0.0f);
    OUT_COLOR = color;
	EmitVertex();

	gl_Position = CAMERAINFO.data.proj * (viewCenter + vec4(extent.x, extent.y, 0.0f, 0.0f));
	OUT_TEXTURE_ID = textureID;
    OUT_UV = vec2(1.0f, 0.0f);
    OUT_COLOR = color;
	EmitVertex();

    gl_Position = CAMERAINFO.data.proj * (viewCenter + vec4(-extent.x, -extent.y, 0.0f, 0.0f));
	OUT_TEXTURE_ID = textureID;
    OUT_UV = vec2(0.0f, 1.0f);
    OUT_COLOR = color;
	EmitVertex();

    gl_Position = CAMERAINFO.data.proj * (viewCenter + vec4(extent.x, -extent.y, 0.0f, 0.0f));
	OUT_TEXTURE_ID = textureID;
    OUT_UV = vec2(1.0f, 1.0f);
    OUT_COLOR = color;
	EmitVertex();

	EndPrimitive();
}
void main()
{
    uint objectID = IN_ID[0];
    GizmoBillboardInfo info = GIZMO_DRAW_DATA.worldBillboards[objectID]; 
    Emit(info.center, info.extent, info.textureID, info.color);
}
#endif

#ifdef FRAGMENT_SHADER
#include "../common/common.glsl"

layout(location = 0) in flat uint IN_TEXTURE_ID;
layout(location = 1) in vec2 IN_UV;
layout(location = 2) in vec4 IN_COLOR;
layout (location = 0) out vec4 OUT_COLOR;

void main()
{
    OUT_COLOR = IN_COLOR;
    if(IN_TEXTURE_ID != 0) OUT_COLOR *= pow(GetTex2D(IN_TEXTURE_ID, IN_UV, 0), vec4(1.0/2.2));         

    if(OUT_COLOR.w < 0.00001f) discard;
}

#endif

