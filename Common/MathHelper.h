#pragma once
#include <DirectXMath.h>
using namespace DirectX;


namespace MathUtils
{
	inline XMMATRIX Mul(const XMMATRIX& mA, const XMMATRIX& mB)
	{
		return XMMatrixMultiply(mA, mB);
	}

	inline XMFLOAT4X4 Mul(const XMFLOAT4X4& a, const XMFLOAT4X4& b)
	{
		XMFLOAT4X4 result;
		const XMMATRIX mA = XMLoadFloat4x4(&a);
		const XMMATRIX mB = XMLoadFloat4x4(&b);
		XMStoreFloat4x4(&result, XMMatrixMultiply(mA, mB));
		return result;
	}

	inline XMFLOAT4 Mul(const XMFLOAT4& a, const XMFLOAT4& b)
	{
		XMFLOAT4 result;
		const XMVECTOR vA = XMLoadFloat4(&a);
		const XMVECTOR vB = XMLoadFloat4(&b);
		XMStoreFloat4(&result, XMVectorMultiply(vA, vB));
		return result;
	}

	inline XMVECTOR Mul(const XMVECTOR& vA, const XMVECTOR& vB)
	{
		return XMVectorMultiply(vA, vB);
	}
	
	inline XMFLOAT3 MulInPlace(XMFLOAT3& a, const XMFLOAT3& b)
	{
		XMStoreFloat3(&a, XMVectorMultiply(XMLoadFloat3(&a), XMLoadFloat3(&b)));
		return a;
	}

	inline XMFLOAT4 MulInPlace(XMFLOAT4& a, const XMFLOAT4& b)
	{
		XMStoreFloat4(&a, XMVectorMultiply(XMLoadFloat4(&a), XMLoadFloat4(&b)));
		return a;
	}

	inline XMFLOAT3 Add(const XMFLOAT3& a, const XMFLOAT3& b)
	{
		XMFLOAT3 result;
		XMStoreFloat3(&result, XMVectorAdd(XMLoadFloat3(&a), XMLoadFloat3(&b)));
		return result;
	}

	inline XMFLOAT3& AddInPlace(XMFLOAT3& a, const XMFLOAT3& b)
	{
		XMStoreFloat3(&a, XMVectorAdd(XMLoadFloat3(&a), XMLoadFloat3(&b)));
		return a;
	}

	inline XMFLOAT4 Add(const XMFLOAT4& a, const XMFLOAT4& b)
	{
		XMFLOAT4 result;
		XMStoreFloat4(&result, XMVectorAdd(XMLoadFloat4(&a), XMLoadFloat4(&b)));
		return result;
	}

	inline XMFLOAT4& AddInPlace(XMFLOAT4& a, const XMFLOAT4& b)
	{
		XMStoreFloat4(&a, XMVectorAdd(XMLoadFloat4(&a), XMLoadFloat4(&b)));
		return a;
	}

	// TRS 행렬 생성
	inline XMMATRIX CreateTRS(const XMFLOAT3& pos, const XMFLOAT4& rot, const XMFLOAT3& scale)
	{
		XMMATRIX mScale = XMMatrixScaling(scale.x, scale.y, scale.z);
		XMMATRIX mRot = XMMatrixRotationQuaternion(XMLoadFloat4(&rot));
		XMMATRIX mTrans = XMMatrixTranslation(pos.x, pos.y, pos.z);

		return XMMatrixMultiply(XMMatrixMultiply(mScale, mRot), mTrans);
	}

	inline XMMATRIX LookAtLH(const XMFLOAT3& eye, const XMFLOAT3& look, const XMFLOAT3& up)
	{
		return XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&look), XMLoadFloat3(&up));
	}

	inline XMMATRIX PerspectiveLH(float angle, float aspect, float nearZ, float farZ)
	{
		return XMMatrixPerspectiveFovLH(angle, aspect, nearZ, farZ);
	}

	inline XMMATRIX OrthographicLH(float width, float height, float nearZ, float farZ)
	{
		return XMMatrixOrthographicLH(width, height, nearZ, farZ);
	}

	inline XMMATRIX OrthographicOffCenterLH(float viewLeft, float viewRight, float viewBottom, float viewTop, float nearZ, float farZ)
	{
		return XMMatrixOrthographicOffCenterLH(viewLeft, viewRight, viewBottom, viewTop, nearZ, farZ);
	}

	// Quaternion from Euler (radian)
	inline XMFLOAT4 QuatFromEular(float pitch, float yaw, float roll)
	{
		XMVECTOR quat = XMQuaternionRotationRollPitchYaw(pitch, yaw, roll);
		XMFLOAT4 result;
		XMStoreFloat4(&result, quat);
		return result;
	}

	inline XMFLOAT3 Normalize(const XMFLOAT3& v)
	{
		XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&v));
		XMFLOAT3 result;
		XMStoreFloat3(&result, n);
		return result;
	}

	inline XMFLOAT4 Normalize(const XMFLOAT4& v)
	{
		XMVECTOR n = XMVector4Normalize(XMLoadFloat4(&v));
		XMFLOAT4 result;
		XMStoreFloat4(&result, n);
		return result;
	}

	inline float Dot(const XMFLOAT3& a, const XMFLOAT3& b)
	{
		return XMVectorGetX(XMVector3Dot(XMLoadFloat3(&a), XMLoadFloat3(&b)));
	}

	inline float Dot(const XMFLOAT4& a, const XMFLOAT4& b)
	{
		return XMVectorGetX(XMVector4Dot(XMLoadFloat4(&a), XMLoadFloat4(&b)));
	}

	inline XMFLOAT3 Cross(const XMFLOAT3& a, const XMFLOAT3& b)
	{
		XMVECTOR c = XMVector3Cross(XMLoadFloat3(&a), XMLoadFloat3(&b));
		XMFLOAT3 result;
		XMStoreFloat3(&result, c);
		return result;
	}

	inline XMFLOAT4X4 Identity()
	{
		XMFLOAT4X4 I;
		XMMATRIX mI = XMMatrixIdentity();
		XMStoreFloat4x4(&I, mI);
		return I;
	}

	inline XMFLOAT4X4 Inverse(const XMFLOAT4X4& mat)
	{
		XMFLOAT4X4 inverse;
		XMMATRIX mInverse = XMMatrixInverse(nullptr, XMLoadFloat4x4(&mat));
		XMStoreFloat4x4(&inverse, mInverse);
		return inverse;
	}	

	inline XMFLOAT4X4 Inverse(XMVECTOR& determinant, const XMFLOAT4X4& mat)
	{
		XMFLOAT4X4 inverse;
		XMMATRIX mInverse = XMMatrixInverse(&determinant, XMLoadFloat4x4(&mat));
		XMStoreFloat4x4(&inverse, mInverse);
		return inverse;
	}

	inline void DecomposeMatrix(const XMFLOAT4X4 matrix, XMFLOAT3& outTranslation, XMFLOAT4& outRotation, XMFLOAT3& outScale)
	{
		XMVECTOR scale, rot, trans;
		XMMATRIX mTM = XMLoadFloat4x4(&matrix);
		XMMatrixDecompose(&scale, &rot, &trans, mTM);

		XMStoreFloat3(&outTranslation, trans);
		XMStoreFloat4(&outRotation, rot);
		XMStoreFloat3(&outScale, scale);
	}

	inline XMFLOAT4X4 RemovePivot(const XMFLOAT4X4& mat_local, const XMFLOAT3& pivot)
	{
		XMFLOAT4X4 result;
		XMMATRIX   mResult;
		const XMMATRIX mToPivotOrigin   = XMMatrixTranslation(-pivot.x, -pivot.y, -pivot.z);
		const XMMATRIX mFromPivotOrigin = XMMatrixTranslation(pivot.x, pivot.y, pivot.z);
		const XMMATRIX mLocalTM         = XMLoadFloat4x4(&mat_local);
		mResult = XMMatrixMultiply(XMMatrixMultiply(mFromPivotOrigin, mLocalTM), mToPivotOrigin);
		XMStoreFloat4x4(&result, mResult);
		return result;
	}

	inline XMFLOAT3 Lerp3(const XMFLOAT3& a, const XMFLOAT3& b, float t)
	{
		XMVECTOR A = XMLoadFloat3(&a);
		XMVECTOR B = XMLoadFloat3(&b);
		XMVECTOR R = XMVectorLerp(A, B, t);
		XMFLOAT3 out; XMStoreFloat3(&out, R);
		return out;
	}

	inline XMFLOAT4 Slerp4(const XMFLOAT4& a, const XMFLOAT4& b, float t)
	{
		XMVECTOR A = XMLoadFloat4(&a);
		XMVECTOR B = XMLoadFloat4(&b);
		XMVECTOR R = XMQuaternionSlerp(A, B, t);
		XMFLOAT4 out; XMStoreFloat4(&out, R);
		return out;
	}
}