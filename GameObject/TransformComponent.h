#pragma once
#include "Component.h"
#include "MathHelper.h"
using namespace MathUtils;

enum class TransformPivotPreset
{
	BottomCenter,
	Center,
	BoundingPivot
};

class TransformComponent : public Component
{
	friend class Editor;
public:

	static constexpr const char* StaticTypeName = "TransformComponent";
	const char* GetTypeName() const override;

	TransformComponent() : Component(), m_Position(0, 0, 0), m_Rotation(0, 0, 0, 1), m_Scale(1, 1, 1), m_IsDirty(false), m_Parent(nullptr)
	{
		m_LocalMatrix = Identity();
		m_WorldMatrix = Identity();
	}
	virtual ~TransformComponent();

	TransformComponent* GetParent() const { return m_Parent; }

	void SetParent(TransformComponent* newParent);
	void DetachFromParent();

	void SetParentKeepLocal(TransformComponent* newParent);
	void DetachFromParentKeepLocal();

	void SetParentKeepWorld(TransformComponent* newParent);
	void DetachFromParentKeepWorld();
	
	

	void AddChild(TransformComponent* child);
	void RemoveChild(TransformComponent* child);

	void SetPosition(const XMFLOAT3& pos) { m_Position = pos; SetDirty(); }
	void SetRotation(const XMFLOAT4& rot) { m_Rotation = rot; SetDirty(); }
	void SetRotationEuler(const XMFLOAT3& rot); //
	void SetScale(const XMFLOAT3& scale) { m_Scale = scale; SetDirty(); }
	
	const XMFLOAT3& GetPosition() const { return m_Position; }
	const XMFLOAT4& GetRotation() const { return m_Rotation; }
	const XMFLOAT3& GetWorldPos();
	const XMFLOAT3& GetScale() const { return m_Scale; }

	void Translate(const XMFLOAT3& delta);
	void Translate(const float x, const float y, const float z);

	void Rotate(float pitch, float yaw, float roll);
	void Rotate(const XMFLOAT4& rot);

	XMFLOAT3 GetForward() const;

	const XMFLOAT4X4& GetWorldMatrix();
	const XMFLOAT4X4& GetLocalMatrix();

	XMFLOAT4X4 GetInverseWorldMatrix();

	void SetPivotPreset(TransformPivotPreset preset, const XMFLOAT3& size);
	XMFLOAT3 GetPivotPoint() const { return m_Pivot; }

	void Update (float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	//void Serialize  (nlohmann::json& j) const override;
	void Deserialize(const nlohmann::json& j) override;

	std::vector<TransformComponent*>& GetChildrens()
	{
		return m_Children;
	}
private:
	void SetDirty()
	{
		m_IsDirty = true;
		for (auto* child : m_Children)
		{
			child->SetDirty();
		}
	}

	void UpdateMatrices();

private:
	XMFLOAT3 m_Position = { 0.0f, 0.0f, 0.0f };
	XMFLOAT4 m_Rotation = { 0.0f, 0.0f, 0.0f, 1.0f }; //quaternion
	XMFLOAT3 m_Scale    = { 1.0f, 1.0f, 1.0f };
	XMFLOAT3 m_WorldPos = { 0.0f,0.0f,0.0f };

	XMFLOAT3 m_Pivot    = { 0.0f, 0.0f, 0.0f };

	TransformComponent* m_Parent = nullptr;
	std::vector<TransformComponent*> m_Children;

	XMFLOAT4X4 m_LocalMatrix;
	XMFLOAT4X4 m_WorldMatrix;
	bool m_IsDirty = true;

};

