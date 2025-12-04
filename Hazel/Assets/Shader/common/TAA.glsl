vec2 CalculateVelocity(vec4 pos, vec4 prevPos){
    vec4 curNDC  = CAMERAINFO.data.proj * CAMERAINFO.data.view * pos;
    curNDC /= curNDC.w;
    vec2 curUV = curNDC.xy * 0.5 + 0.5;
    vec4 prevNDC = CAMERAINFO.data.prevProj * CAMERAINFO.data.prevView * prevPos;
    prevNDC /= prevNDC.w;
    vec2 prevUV = prevNDC.xy * 0.5 + 0.5;
    return vec2(prevUV.xy - curUV.xy);
}