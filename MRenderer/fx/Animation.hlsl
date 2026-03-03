#ifndef ANIMATION_HLSL
#define ANIMATION_HLSL

#include "BaseBuffer.hlsl"

float4 ApplyBone(float4 pos, uint idx)
{
    if (idx >= count)       //bonecount임
        return pos; // identity 적용한 것과 동일
    return mul(pos, bones[idx]);
}

float4 Skinning(float4 pos, float4 weight, uint4 index)
{
    if (count == 0)
        return pos;
    float4 skinVtx;

    float4 v0 = ApplyBone(pos, index.r);
    float4 v1 = ApplyBone(pos, index.g);
    float4 v2 = ApplyBone(pos, index.b);
    float4 v3 = ApplyBone(pos, index.a);

    skinVtx = v0 * weight.r + v1 * weight.g + v2 * weight.b + v3 * weight.a;
    
    return skinVtx;
    
}



#endif