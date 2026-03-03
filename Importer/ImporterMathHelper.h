#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "MathHelper.h"

inline aiVector3D Cross3(const aiVector3D& V1, const aiVector3D& V2)
{
	return aiVector3D(
		V1.y * V2.z - V1.z * V2.y,
		V1.z * V2.x - V1.x * V2.z,
		V1.x * V2.y - V1.y * V2.x
	);
}

inline float Dot3(const aiVector3D& V1, const aiVector3D& V2)
{
	return V1.x * V2.x + V1.y * V2.y + V1.z * V2.z;
}

inline DirectX::XMFLOAT4X4 ToXMFLOAT4X4(const aiMatrix4x4& m)
{
	DirectX::XMFLOAT4X4 out{};
	out._11 = m.a1; out._12 = m.a2; out._13 = m.a3; out._14 = m.a4;
	out._21 = m.b1; out._22 = m.b2; out._23 = m.b3; out._24 = m.b4;
	out._31 = m.c1; out._32 = m.c2; out._33 = m.c3; out._34 = m.c4;
	out._41 = m.d1; out._42 = m.d2; out._43 = m.d3; out._44 = m.d4;

	XMMATRIX xm = XMLoadFloat4x4(&out);
	xm = XMMatrixTranspose(xm);
	XMStoreFloat4x4(&out, xm);

	return out;
}

inline aiVector3D TransformPoint(const aiMatrix4x4& m, const aiVector3D& p)
{
	// row-major 기준: [x y z 1] * M
	aiVector3D r;
	r.x = p.x * m.a1 + p.y * m.b1 + p.z * m.c1 + m.d1;
	r.y = p.x * m.a2 + p.y * m.b2 + p.z * m.c2 + m.d2;
	r.z = p.x * m.a3 + p.y * m.b3 + p.z * m.c3 + m.d3;
	return r;
}