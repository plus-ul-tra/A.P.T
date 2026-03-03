#pragma once
#include "Component.h"
#include <DirectXMath.h>

class BoxColliderComponent : public Component
{
public:
	static constexpr const char* StaticTypeName = "BoxColliderComponent";
	const char* GetTypeName() const override;

	void Start() override;
	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void SetLocalBounds(const DirectX::XMFLOAT3& min, const DirectX::XMFLOAT3& max);
	bool HasBounds() const { return m_HasBounds; }
	bool BuildWorldBounds(DirectX::XMFLOAT3& outMin, DirectX::XMFLOAT3& outMax) const;
	bool IntersectsRay(const DirectX::XMFLOAT3& rayOrigin, const DirectX::XMFLOAT3& rayDir, float& outT) const;


	void SetLocalMin(const DirectX::XMFLOAT3& localMin) { m_LocalMin = localMin; }
	void SetLocalMax(const DirectX::XMFLOAT3& localMax) { m_LocalMax = localMax; }
	const DirectX::XMFLOAT3& GetLocalMin() const { return m_LocalMin; }
	const DirectX::XMFLOAT3& GetLocalMax() const { return m_LocalMax; }

private:
	void TryInitializeFromMesh();

	DirectX::XMFLOAT3 m_LocalMin{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 m_LocalMax{ 0.0f, 0.0f, 0.0f };
	bool m_HasBounds = false;
};

