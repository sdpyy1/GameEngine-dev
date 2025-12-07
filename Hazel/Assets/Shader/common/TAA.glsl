vec2 CalculateVelocity(vec4 pos, vec4 prevPos){
    vec4 curNDC  = CAMERAINFO.data.projNoJetter * CAMERAINFO.data.view * pos;
    curNDC /= curNDC.w;
    vec2 curUV = curNDC.xy * 0.5 + 0.5;
    
    vec4 prevNDC = CAMERAINFO.data.projNoJetter * CAMERAINFO.data.prevView * prevPos;
    prevNDC /= prevNDC.w;
    vec2 prevUV = prevNDC.xy * 0.5 + 0.5;
    return vec2(curUV.xy - prevUV.xy);
}

vec2 GetClosestFragment(vec2 uv, vec2 texSize, texture2D depthTexture)
{
    vec2 k = texSize; 
    
    float depth0 = texture(sampler2D(depthTexture, SAMPLER[0]), clamp(uv - k, 0.0, 1.0)).r;
    float depth1 = texture(sampler2D(depthTexture, SAMPLER[0]), clamp(uv + vec2(k.x, -k.y), 0.0, 1.0)).r;
    float depth2 = texture(sampler2D(depthTexture, SAMPLER[0]), clamp(uv + vec2(-k.x, k.y), 0.0, 1.0)).r;
    float depth3 = texture(sampler2D(depthTexture, SAMPLER[0]), clamp(uv + k, 0.0, 1.0)).r;
    
    vec4 neighborhood = vec4(depth0, depth1, depth2, depth3);

    #define COMPARE_DEPTH(a, b) step(a, b)   // TODO: 只设置了正向Z


    vec3 result = vec3(0.0, 0.0, texture(sampler2D(depthTexture, SAMPLER[0]), uv).r);
    
    result = mix(result, vec3(-1.0, -1.0, neighborhood.x), COMPARE_DEPTH(neighborhood.x, result.z));
    result = mix(result, vec3( 1.0, -1.0, neighborhood.y), COMPARE_DEPTH(neighborhood.y, result.z));
    result = mix(result, vec3(-1.0,  1.0, neighborhood.z), COMPARE_DEPTH(neighborhood.z, result.z));
    result = mix(result, vec3( 1.0,  1.0, neighborhood.w), COMPARE_DEPTH(neighborhood.w, result.z));

    #undef COMPARE_DEPTH
    return uv + result.xy * k;
}