#pragma once
#include "Component.h"
#include "IEventListener.h"
#include "Event.h"
#include <vector>

class PlayerComponent;
class TransformComponent;


class VendingComponent : public Component, public IEventListener
{
public:
	static constexpr const char* StaticTypeName = "VendingComponent";
	const char* GetTypeName() const override;

	VendingComponent();
	virtual ~VendingComponent();

	void Start() override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;
	void SetDistance(const float& value) { m_Distance = value; }
	const float& GetDistance() const { return m_Distance; }
	bool Clicked(const Events::MouseState* mouseData);
private:
	
	PlayerComponent* FindPlayerComponent() const;
	bool IsClickedThisVending(const Events::MouseState& mouseData) const; // ray

	PlayerComponent* m_Player = nullptr;
	float m_Distance = 2.0f;
};