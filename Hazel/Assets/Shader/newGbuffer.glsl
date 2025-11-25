#version 450 core
#include "common/common.glsl"
#ifdef VERTEX_SHADER
void main()
{


}
#endif

#ifdef FRAGMENT_SHADER
layout (location = 0) out vec4 G_BUFFER_DIFFUSE_ROUGHNESS;
layout (location = 1) out vec4 G_BUFFER_NORMAL_METALLIC;
void main()
{
    G_BUFFER_DIFFUSE_ROUGHNESS = vec4(1.0, 1.0, 1.0, 1.0);
    G_BUFFER_NORMAL_METALLIC   = vec4(1.0, 1.0, 1.0, 1.0);

}
#endif
