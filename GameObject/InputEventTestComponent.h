#pragma once
#include "Component.h"
#include "Event.h"

class InputEventTestComponent : public Component, public IEventListener
{
public:
	static constexpr const char* StaticTypeName = "InputEventTestComponent";
	const char* GetTypeName() const override;

	InputEventTestComponent();
	virtual ~InputEventTestComponent();

	void Start() override;
	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;
	bool ShouldHandleEvent(EventType type, const void* data) override;

private:
	void LogEvent(const char* label, const Events::MouseState* mouseData) const;
	bool IsWithinBounds(const POINT& pos) const;
};
