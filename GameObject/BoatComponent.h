#pragma once
#include "Component.h"

class BoatComponent : public Component {

public:
	static constexpr const char* StaticTypeName = "BoatComponent";
	const char* GetTypeName() const override;

	BoatComponent() = default;
	virtual ~BoatComponent() = default;

	void SetHeight(const float& value) { m_Height = value; }
	const float& GetHeight() const { return m_Height; }

	void SetSpeed(const float& value) { m_Speed = value; }
	const float& GetSpeed() const { return m_Speed; }

	void SetVertical(const bool& value) { m_Vertical = value; }
	const bool& GetVertical() const { return m_Vertical; }

	void Start() override;
	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;
private:
	void SelfBob(float dTime);


	float m_BobTime = 0.0f;
	float m_Height = 0.001f;
	float m_Speed = 1.0f;
	bool m_Vertical = true;
};