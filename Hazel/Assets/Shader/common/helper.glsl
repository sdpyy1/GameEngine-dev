#ifndef HELPER_GLSL
#define HELPER_GLSL
vec2 ScreenPixToUV(ivec2 pixel, ivec2 totalPixels)
{
    return (pixel + vec2(0.5f)) / vec2(totalPixels);
}
#endif