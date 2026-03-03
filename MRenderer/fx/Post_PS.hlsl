#include "BaseBuffer.hlsl"
#include "Lights.hlsl"

// tilt shift를 위한 화면 위 아래를 늘이는 함수
float2 WarpTopExpand(float2 uv, float amount, float power)
{
    float t = pow(saturate(uv.y), power);
    float scale = 1.0 + t * amount;
    uv.x = (uv.x - 0.5) / scale + 0.5;
    return uv;
}

// Emissive를 퍼지도록 픽셀을 미는 함수
float4 SampleEmissiveRadial(Texture2D tex, float2 uv, float2 r)
{
    float2 o = float2(r.x, r.y);
    float4 c = 0;
    c += tex.Sample(smpClamp, uv);
    c += tex.Sample(smpClamp, uv + float2(r.x, 0));
    c += tex.Sample(smpClamp, uv + float2(-r.x, 0));
    c += tex.Sample(smpClamp, uv + float2(0, r.y));
    c += tex.Sample(smpClamp, uv + float2(0, -r.y));
    c += tex.Sample(smpClamp, uv + o);
    c += tex.Sample(smpClamp, uv - o);
    c += tex.Sample(smpClamp, uv + float2(r.x, -r.y));
    c += tex.Sample(smpClamp, uv + float2(-r.x, r.y));
    return c / 9.0;
}

float2 Rotate2D(float2 v, float a)
{
    float s = sin(a);
    float c = cos(a);
    return float2(c * v.x - s * v.y, s * v.x + c * v.y);
}

float Hash12(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}

// Emissive 원형 방향 밀기
float4 SampleEmissiveDisk(Texture2D tex, float2 uv, float2 r)
{
    float angle = Hash12(uv * screenSize.xy) * 6.28318;
    float4 c = tex.Sample(smpClamp, uv) * 0.4;

    [unroll]
    for (int i = 0; i < 6; i++)
    {
        float t = (i + 0.5) / 6.0;
        float a = angle + t * 6.28318;
        float spread = pow(t, 0.3);
        float2 o = float2(cos(a), sin(a)) * r * spread;
        c += tex.Sample(smpClamp, uv + o);
    }

    return c / 7.0;
}

float4 PS_Main(VSOutput_PU i) : SV_TARGET
{
    // ====== 1. UV Warp (Tilt-Shift) ======
    float warpAmount = 0.15f;
    float warpPower = 3.0f;
    float2 uvW = WarpTopExpand(i.uv, warpAmount, warpPower);
    uvW = saturate(uvW);

    // ====== 2. Scene Sampling & Linear Conversion ======
    float4 RTView = g_RTView.Sample(smpClamp, uvW);
    RTView.rgb = SRGBToLinear(RTView.rgb);

    // ====== 3. Depth & CoC (Depth of Field) ======
    float DepthMap = g_DepthMap.Sample(smpClamp, uvW).r;
    float viewZ = camParams.x * camParams.y / (camParams.y - DepthMap * (camParams.y - camParams.x));
    float coc = saturate(abs(viewZ - camParams.z) / camParams.w);
    
    float2 pixelSize = 1.0 / screenSize.xy;

    // ====== 4. Radial Blur (CoC 기반) ======
    float radialStrength = coc * 6.0;
    float2 radialOffset = pixelSize * radialStrength;
    float angle = Hash12(uvW * screenSize.xy) * 6.28318;
    float2 rRot = Rotate2D(radialOffset, angle);
    float jitter = lerp(0.7, 1.3, Hash12(uvW * screenSize.xy + 17.0));
    
    float4 radialBlur = SampleEmissiveRadial(g_RTView, uvW, rRot * jitter);
    radialBlur.rgb = SRGBToLinear(radialBlur.rgb); // Radial Blur 결과도 Linear 변환 확인

    // ====== 5. Gaussian Blur Layers (DoF) ======
    float4 Blur1 = g_BlurHalf.Sample(smpClamp, uvW);
    float4 Blur2 = g_BlurHalf2.Sample(smpClamp, uvW);
    //float4 Blur3 = g_BlurHalf3.Sample(smpClamp, uvW);
    //float4 Blur4 = g_BlurHalf4.Sample(smpClamp, uvW);
    
    Blur1.rgb = SRGBToLinear(Blur1.rgb);
    Blur2.rgb = SRGBToLinear(Blur2.rgb);
   // Blur3.rgb = SRGBToLinear(Blur3.rgb);
    //Blur4.rgb = SRGBToLinear(Blur4.rgb);
    
    float4 blurCombined =
        Blur1 * 1.0 +
        Blur2 * 0.0;
    float4 dofColor = lerp(RTView, blurCombined, coc);

    // Radial Blur와 DoF Blur 합성
    float radialWeight = saturate(coc * 1.2);
    float4 finalBlur = lerp(dofColor, radialBlur, radialWeight);
         
    // ====== 6. HDR Emissive (Bloom 효과) ======
    float2 texelFull = 1.0 / screenSize.xy;
    float2 texelHalf = texelFull * 6.0;
    float2 texelQuarter = texelFull * 12.0;
    float2 texel8 = texelFull * 20.0;

    float4 e0 = g_RTEmissive.Sample(smpClamp, uvW);
    float core = e0.a;
    e0.rgb *= core;
    
    // Core 강화 시 너무 타지 않게 조절
    float l = max(e0.r, max(e0.g, e0.b));
    e0.rgb = lerp(e0.rgb, 1.0.xxx, saturate(l * 0.8)); // 1.2 -> 0.8로 하향
    e0.rgb *= saturate(core);
    
    float4 e1 = SampleEmissiveDisk(g_RTEmissiveHalf, uvW, texelHalf);
    float4 e2 = SampleEmissiveDisk(g_RTEmissiveHalf2, uvW, texelQuarter);
    float4 e3 = SampleEmissiveDisk(g_RTEmissiveHalf3, uvW, texel8);

    // HDR 환경에서는 가중치 합산 주의 (전체적으로 밝아진 주범)
    float4 emissive = (e0 * 1.5) + (e1 * 0.8) + (e2 * 0.5) + (e3 * 0.3);

    // ====== 7. Final Composition & Post-Process ======
    float4 scene = finalBlur + emissive;
    
    // Saturation 조절 (ToneMap 전 수행)
    scene.rgb = AdjustSaturation(scene.rgb, lights[0].Saturation);

    // ACES ToneMapping (HDR -> LDR 변환)
    scene.rgb = ToneMap_ACES(scene.rgb);
    
    // Contrast 조절 (ACES 이후에는 아주 미세하게 적용)
    // ACES가 이미 대비를 높이므로 기존보다 값을 낮게 책정하세요.
    scene.rgb = AdjustContrast_Luma(scene.rgb, lights[0].Contrast);
    
    // Final Gamma Correction
    scene.rgb = LinearToSRGB(scene.rgb);
    scene.a = 1.0f;

    return scene;
}