#version 450 core
#include "common/common.glsl"
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
layout(points) in;
layout(line_strip, max_vertices = 2) out;
layout(location = 0) in uint IN_ID[];
layout(location = 0) out vec4 OUT_COLOR;

void Emit(vec3 from, vec3 to, vec4 color)
{
	gl_Position = u_CameraData.data.proj * u_CameraData.data.view * vec4(from, 1.0f);
	OUT_COLOR = color;
	EmitVertex();

	gl_Position = u_CameraData.data.proj * u_CameraData.data.view * vec4(to, 1.0f);
	OUT_COLOR = color;
	EmitVertex();

	EndPrimitive();
}

void main()
{
	uint objectID = IN_ID[0];
    GizmoLineInfo info           = GIZMO_DRAW_DATA.lines[objectID];

    Emit(info.from, info.to, info.color);
}
#endif
#ifdef FRAGMENT_SHADER
layout(location = 0) in vec4 IN_COLOR;
layout (location = 0) out vec4 OUT_COLOR;

void main()
{
    OUT_COLOR = IN_COLOR;
}
#endif
