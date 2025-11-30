#version 450 core
#include "common/common.glsl"
#ifdef VERTEX_SHADER
layout(location = 0) in vec3 IN_POS;
layout(location = 0) out vec4 OUT_COLOR;
void main() 
{
    uint objectID               = gl_InstanceIndex;
    GizmoBoxInfo info           = GIZMO_DRAW_DATA.boxes[objectID];

    OUT_COLOR                   = info.color;

    vec4 pos = vec4(IN_POS * info.extent + info.center, 1.0f);
    gl_Position = u_CameraData.data.proj * u_CameraData.data.view * pos;
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
