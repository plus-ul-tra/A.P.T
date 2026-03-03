#pragma once
#include "MathHelper.h"
using namespace MathUtils;

struct Ray
{
	XMFLOAT3 m_Pos;
	XMFLOAT3 m_Dir;
};

inline Ray MakePickRayLH(
	float screenX, float screenY,
	float viewportX, float viewportY,
	float viewportW, float viewportH,
	const XMMATRIX& view,
	const XMMATRIX& proj
)
{
	//ViewProj 역행렬
	const XMMATRIX invViewProj = XMMatrixInverse(nullptr, XMMatrixMultiply(view, proj));

	// screen -> NDC (-1 ~ 1), Y 반전
	const float ndcX = ((screenX - viewportX) / viewportW) * 2.0f - 1.0f;
	const float ndcY = 1.0f - ((screenY - viewportY) / viewportH) * 2.0f;

	// NDC : near(z = 0), far(z = 1) 월드로 변환
	const XMVECTOR nearNdc = XMVectorSet(ndcX, ndcY, 0.0f, 1.0f);
	const XMVECTOR farNdc  = XMVectorSet(ndcX, ndcY, 1.0f, 1.0f);

	XMVECTOR nearWorld = XMVector4Transform(nearNdc, invViewProj);
	XMVECTOR farWorld  = XMVector4Transform(farNdc, invViewProj);

	// 동차 나누기
	nearWorld = XMVectorScale(nearWorld, 1.0f / XMVectorGetW(nearWorld));
	farWorld  = XMVectorScale(farWorld, 1.0f / XMVectorGetW(farWorld));


	XMVECTOR dir = XMVector3Normalize(XMVectorSubtract(farWorld, nearWorld));

	Ray ray{};
	XMStoreFloat3(&ray.m_Pos, nearWorld);
	XMStoreFloat3(&ray.m_Dir, dir);
	return ray;
}

// ray와 y=planeY 평면 교차. 성공하면 t(>=0)와 hitPoint 반환.
inline bool IntersectRayPlaneY(const Ray& ray, float planeY, float& outT, XMFLOAT3& outHit)
{
	// 평면: y = planeY
	const float denom = ray.m_Dir.y;
	if (std::fabs(denom) < 1e-6f)
		return false; // 평행

	const float t = (planeY - ray.m_Pos.y) / denom;
	if (t < 0.0f)
		return false; // 카메라 뒤쪽

	outT = t;
	outHit = {
		ray.m_Pos.x + ray.m_Dir.x * t,
		planeY,
		ray.m_Pos.z + ray.m_Dir.z * t
	};
	return true;
}