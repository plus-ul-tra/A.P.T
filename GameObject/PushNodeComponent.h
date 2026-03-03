#pragma once
#include "Component.h"
#include "IEventListener.h"

class PushNodeComponent : public Component, public IEventListener {

public:
	static constexpr const char* StaticTypeName = "PushNodeComponent";
	const char* GetTypeName() const override;

	PushNodeComponent();
	virtual ~PushNodeComponent() = default;

	void Start() override;

	void Update(float dTime) override;
	void OnEvent(EventType type, const void* data) override;

private:



};