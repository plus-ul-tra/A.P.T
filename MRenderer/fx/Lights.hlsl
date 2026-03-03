#ifndef LIGHTS_HLSL
#define LIGHTS_HLSL
#define PI 3.141592f

#include "BaseBuffer.hlsl"

float3 ToneMap_ACES(float3 x)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;

    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float3 AdjustContrast(float3 color, float contrast)
{
    return saturate((color - 0.5f) * contrast + 0.5f);
}

float3 AdjustContrast_Luma(float3 color, float contrast)
{
    float luma = dot(color, float3(0.2126, 0.7152, 0.0722));

    float newLuma = (luma - 0.5f) * contrast + 0.5f;

    float scale = newLuma / max(luma, 0.0001f);

    return saturate(color * scale);
}


float4 DirectLight(float4 nrm)
{
    float4 diff = 0;   diff.a = 1;
    float4 amb = 0;    amb.a = 1;
    
    float4 N = nrm;
    N.w = 0;
    float4 L = float4(lights[0].viewDir, 0);
        
    //뷰공간으로 정보를 변환.
    N = mul(N, mView);
    L = mul(L, mView);
    
    //각 벡터 노멀라이즈.
    N = normalize(N);
    L = normalize(L);

    
    //조명 계산 
    float4 color = saturate(lights[0].Color);
    diff = max(dot(N, L), 0) * color;
    amb = saturate(lights[0].Color) * 0.1f;
    //float4 amb = 0;


	//포화도 처리 : 광량의 합을 1로 제한합니다.
	return saturate(diff + amb);
}

float4 SpecularLight_Point(float4 pos, float4 nrm, Light light)
{
    float4 spec = 0;
    spec.a = 1;
    
    float3 N = nrm.xyz;
    N = normalize(N);
    
    float3 L = normalize(light.Pos - pos.xyz);

    float3 V = normalize(cameraPos - pos.xyz);
    
    float3 H = normalize(L + V);

    float NdotL = saturate(dot(N, L));
    if (NdotL <= 0)
        return 0;

    float scalar = saturate(dot(H, N));
    
    scalar = pow(scalar, 80.f);
    
    spec.xyz = light.Color.xyz * scalar * NdotL;
    
    return saturate(spec);
}

float4 SpecularLight_Direction(float4 pos, float4 nrm)
{
    float4 spec = 0;
    spec.a = 1;
    
    float3 N = nrm.xyz;
    N = normalize(N);
    
    float3 L = normalize(lights[0].viewDir);

    float3 V = normalize(cameraPos - pos.xyz);
    
    float3 H = normalize(L + V);

    float NdotL = saturate(dot(N, L));
    if (NdotL <= 0)
        
        return 0;
    float scalar = saturate(dot(H, N));
    
    scalar = pow(scalar, 80.f);
    
    spec.xyz = lights[0].Color.xyz * scalar * NdotL;
    
    return saturate(spec);
}

float PCF(float4 smUV)
{
    float2 uv = smUV.xy ;
    float curDepth = smUV.z;

    float offset = 1.0f / 4096.0f;
    float bias = pow(0.5f, 11);

    float sum = 0.0f;

    // 4x4 PCF (0~3)
    [unroll]
    for (int y = 0; y < 4; y++)
    {
        [unroll]
        for (int x = 0; x < 4; x++)
        {
            float2 suv = uv + float2((x - 1) * offset, (y - 1) * offset);
            float smDepth = g_ShadowMap.Sample(smpBorderShadow, suv).r;
            
            sum += (smDepth >= curDepth - bias) ? 1.0f : 0.0f;
        }
    }
    //float shadow = smoothstep(0.0f, 16.0f, sum);
    float shadow = sum / 16.0f;
    return max(shadow, 0.3f);
}

float CastShadow(float4 uv)
{
    uv.xy /= uv.w;

    float shadowDepth = g_ShadowMap.Sample(smpBorderShadow, uv.xy).r;

    if (shadowDepth == 0.0f)
        return 1.0f; // 그림자 없음(또는 밖)

    return PCF(uv);
}

float4 Masking(float4 uv)
{
    uv.xy /= uv.w;
    
    float4 mask = g_Mask_Wall.Sample(smpBorder, uv.xy);
    
    
    
    return mask;
}

void BuildTBN(
    float3 inT,
    float3 inN,
    float handedness,
    float4 texN,
    out float3 T,
    out float3 B,
    out float3 N)
{
    inN = normalize(inN);
    inT = normalize(inT - dot(inT, inN) * inN);
    B = cross(inN, inT) * handedness;

    T = normalize(inT);
    B = normalize(B);
    N = normalize(inN);

    float3 n = texN.xyz * 2 - 1;
    //n.y = -n.y; // GL → DX
        
    float3x3 mTBN = float3x3(T, B, N);
    n = normalize(mul(n, mTBN));

    N = n;
}


// UE style helpers -----------------------------------------------------------

float Pow5(float x)
{
    float x2 = x * x;
    return x2 * x2 * x;
}

float4 UE_Fresnel_Schlick(float4 F0, float vdoth)
{
    return F0 + (1.0 - F0) * Pow5(1.0 - vdoth);
}

// UE4/5: GGX / Trowbridge-Reitz NDF
float4 UE_D_GGX(float ndoth, float4 roughness)
{
    float4 a = roughness * roughness; // alpha
    float4 a2 = a * a;
    float4 d = (ndoth * ndoth) * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

// UE4/5: Smith Schlick-GGX geometry term
float4 UE_G_SchlickGGX(float ndotx, float4 k)
{
    return ndotx / (ndotx * (1.0 - k) + k);
}

float4 UE_G_Smith(float ndotv, float ndotl, float4 roughness)
{
   
    float4 r = roughness + 1.0;
    // UE: k = (r+1)^2 / 8
    float4 k = (r * r) * 0.125;
    float4 gv = UE_G_SchlickGGX(ndotv, k);
    float4 gl = UE_G_SchlickGGX(ndotl, k);
    return gv * gl;
}


// F0 from metallic workflow (UE style)
float4 ComputeF0(float4 baseColor, float4 metallic, float specular)
{
    float4 dielectricF0 = 0.08 * specular;
    return lerp(dielectricF0, baseColor, metallic);
}

struct BRDFResult
{
    float4 diffuse;
    float4 specular;
};

BRDFResult BRDF_UE_Direct(float4 viewNrm, float4 viewPos, float4 viewLitDir,
                          float4 baseColor, float4 metallic, float4 roughness,
                          float4 ao, float specularParam) // specularParam: UE "Specular" (default 0.5)
{
    BRDFResult o;
    o.diffuse = 0;
    o.specular = 0;

    float4 P = viewPos;
    P.w = 1;
    float4 N = normalize(viewNrm);
    float3 V3 = -P.xyz;
    V3 = (dot(V3, V3) < 1e-8) ? float3(0, 0, 1) : normalize(V3);
    float4 V = float4(V3, 0);
    float4 L = normalize(viewLitDir);

    float4 H = normalize(V + L);

    float ndotl = saturate(dot(N, L));
    float ndotv = saturate(dot(N, V));
    float ndoth = saturate(dot(N, H));
    float vdoth = saturate(dot(V, H));

    // avoid divide by 0
    ndotl = max(ndotl, 1e-4);
    ndotv = max(ndotv, 1e-4);

    // F0
    float4 F0 = ComputeF0(baseColor, metallic, specularParam);

    // Specular microfacet
    float4 D = UE_D_GGX(ndoth, roughness);
    float4 G = UE_G_Smith(ndotv, ndotl, roughness);
    float4 F = UE_Fresnel_Schlick(F0, vdoth);
         
    float4 spec = (D * G) * F / (4.0 * ndotv * ndotl);

    // Diffuse (Lambert) with energy conservation (UE style)
    float4 kd = (1.0 - F) * (1.0 - metallic); // metals have no diffuse
    float4 diff = kd * baseColor / PI;

    diff *= ao;

    o.diffuse = diff;
    o.specular = spec;
    return o;
}

float4 UE_DirectionalLighting(float4 viewPos, float4 viewNrm, float4 viewLitDir,
                              float4 base, float4 ao, float4 metallic, float4 roughness,
                              float specularParam)
{
    float4 P = viewPos;
    float4 N = viewNrm;
    float4 L = normalize(viewLitDir);
    L.w = 0;
    //float4 V = normalize(-P);

    BRDFResult brdf = BRDF_UE_Direct(N, P, L, base, metallic, roughness, ao, specularParam);
        
    float4 radianceDiff = lights[0].Color * lights[0].Intensity;
    float4 radianceSpec = lights[0].Color * lights[0].Intensity;

    float ndotl = saturate(dot(normalize(N), L));
    
    float4 albedo = (base * (1.0 - metallic)) * ao;

    float4 color = (brdf.diffuse * radianceDiff + brdf.specular * radianceSpec) * ndotl;
    color.a = 1;

    float4 amb = lights[0].Color * 0.1f /*주변광 임시로 0.1배*/ * lights[0].Intensity * albedo;    amb.a = 1;
    color += amb;
    color.a = 1;
    //return float4(lights[0].Intensity, lights[0].Intensity, lights[0].Intensity, 1);
    
    return color;
}

float3 SRGBToLinear(float3 c)
{
    return pow(saturate(c), 2.2f);
}

float3 LinearToSRGB(float3 c)
{
    return pow(saturate(c), 1.0f / 2.2f);
}

//채도 조절
float3 AdjustSaturation(float3 color, float saturation)
{
    // saturation: 1 = 원본, 0 = 흑백, 1보다 크면 더 쨍함
    float luma = dot(color, float3(0.0656, 0.7152, 0.0722));
    return lerp(luma.xxx, color, saturation);
}

// Point Light ----------------------------------------------------------------
float4 UE_PointLighting_FromLight(float4 viewPos, float4 viewNrm, Light lit,
                                  float4 base, float4 ao, float4 metallic, float4 roughness,
                                  float specularParam)
{
    float4 P = viewPos; // view space
    float4 N = normalize(viewNrm);

    // light position: world -> view 변환
    float3 litPosV = mul(float4(lit.Pos, 1.0f), mView).xyz;

    float3 toL = litPosV - P.xyz;
    float dist2 = dot(toL, toL);
    float dist = sqrt(max(dist2, 1e-8));

    float3 L = toL / dist; // view space

    // attenuation (range 기반)
    float att = 1.0f / max(dist2, 1e-4);

    float s = saturate(1.0f - dist / max(lit.Range, 1e-4));
    att *= (s * s);
    
    roughness = max(roughness, 0.5f);

    BRDFResult brdf = BRDF_UE_Direct(float4(N.xyz, 0), float4(P.xyz, 1), float4(L, 0),
                                     base, metallic, roughness, ao, specularParam);

    float4 radiance = lit.Color * max(lit.Intensity,0);

    float ndotl = saturate(dot(N.xyz, L));

    float4 color = (brdf.diffuse + brdf.specular) * radiance * ndotl * att;
    
    color.a = 1;
    
    return color;
}

//spotlight
float4 UE_SpotLighting_FromLight(float4 viewPos, float4 viewNrm, Light lit,
                                 float4 base, float4 ao, float4 metallic, float4 roughness,
                                 float specularParam)
{
    float4 P = viewPos; // view space
    float4 N = normalize(viewNrm);

// light position: world -> view
    float3 litPosV = mul(float4(lit.Pos, 1.0f), mView).xyz;


    float3 toL = litPosV - P.xyz;
    float dist2 = dot(toL, toL);
    float dist = sqrt(max(dist2, 1e-8));
    float3 L = toL / dist; // view space (P -> Light)


// ----------------------------
// 거리 감쇠 (attenuation radius 기반)
    float r = max(lit.Range, 1e-4);
    float invR2 = 1.0f / (r * r);


// dist2가 r^2에 가까워질수록 0으로 감쇠
    float att = saturate(1.0f - dist2 * invR2);
    att *= att; // 부드럽게


    //float fadeWidth = max(lit.AttenuationRadius * 0.1f, 0.01f); // 10% 정도 페이드
    //float cutoff = smoothstep(lit.AttenuationRadius, lit.AttenuationRadius - fadeWidth, dist);
    //att *= cutoff;
    
    float cutoff = step(dist, lit.AttenuationRadius);
    att *= cutoff;
    
// ----------------------------
// 각도 감쇠 (Spot)
// lit.Dir : world space spotlight direction (Light가 바라보는 방향)
    float3 litDirV = normalize(lit.viewDir);


// spotlight axis는 Light -> Pixel 방향이어야 함
// 현재 L은 Pixel -> Light 이므로 반대로 뒤집어야 함
    float3 lightToP = -L;


    float spotCos = dot(lightToP, litDirV);

// lit.InnerAngle / lit.OuterAngle 가 "라디안"이라고 가정
    float innerCos = cos(lit.SpotInnerAngle);
    float outerCos = cos(lit.SpotOutterAngle);


    float spot = smoothstep(outerCos, innerCos, spotCos);


// ----------------------------
    BRDFResult brdf = BRDF_UE_Direct(float4(N.xyz, 0), float4(P.xyz, 1), float4(L, 0),
base, metallic, roughness, ao, specularParam);


    float4 radiance = lit.Color * max(lit.Intensity, 0);


    float ndotl = saturate(dot(N.xyz, L));


    float4 color = (brdf.diffuse + brdf.specular) * radiance * ndotl * att * spot;
    color.a = 1;
    return color;
}

#endif