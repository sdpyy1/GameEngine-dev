#ifndef SVGF_GLSL
#define SVGF_GLSL

const float gaussKernel5x5[25] = float[25](
    1, 4,  7,  4,  1,
    4, 16, 26, 16, 4,
    7, 26, 41, 26, 7, 
    4, 16, 26, 16, 4,    
    1, 4,  7,  4,  1
);  // 273

float GetLuminanceWeight(float centerLuminance, float sampleLuminance, float variance)
{
    return min(exp(-abs(centerLuminance - sampleLuminance) / (4.0 * sqrt(variance) + 0.001)), 1.0f);  // 亮度项
}




#endif