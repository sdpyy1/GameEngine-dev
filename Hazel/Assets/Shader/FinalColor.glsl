#version 450 core
#ifdef VERTEX_SHADER
vec2 NDC[3] = vec2[](
    vec2(-1.0, -1.0), 
    vec2( 3.0, -1.0), 
    vec2(-1.0,  3.0) 
);
layout(location = 0) out vec2 out_texCoord;
void main(){

	gl_Position = vec4(NDC[gl_VertexIndex],0,1);
    out_texCoord = (NDC[gl_VertexIndex] + 1.0) * 0.5;
}
#endif

#ifdef FRAGMENT_SHADER
// Based on http://www.oscars.org/science-technology/sci-tech-projects/aces
vec3 ACESTonemap(vec3 color)
{
	mat3 m1 = mat3(
		0.59719, 0.07600, 0.02840,
		0.35458, 0.90834, 0.13383,
		0.04823, 0.01566, 0.83777
	);
	mat3 m2 = mat3(
		1.60475, -0.10208, -0.00327,
		-0.53108, 1.10813, -0.07276,
		-0.07367, -0.00605, 1.07602
	);
	vec3 v = m1 * color;
	vec3 a = v * (v + 0.0245786) - 0.000090537;
	vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
	return clamp(m2 * (a / b), 0.0, 1.0);
}
vec3 GammaCorrect(vec3 color, float gamma)
{
	return pow(color, vec3(1.0f / gamma));
}
layout(location = 0) in vec2 in_texCoord;
layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform texture2D lightRes;
layout(set = 0, binding = 1) uniform texture2D BloomRes;
layout(set = 1, binding = 0) uniform sampler SAMPLER[];
void main(){

	float BloomScale = 0.2;  // TODO: Setting


	vec3 finalColor = texture(sampler2D(lightRes,SAMPLER[0]), in_texCoord).rgb;
	finalColor += texture(sampler2D(BloomRes,SAMPLER[0]), in_texCoord).rgb * BloomScale;
	finalColor = ACESTonemap(finalColor);

	const float gamma = 2.2;

	finalColor = GammaCorrect(finalColor, gamma);
	out_color = vec4(finalColor, 1.0);
}

#endif
