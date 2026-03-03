#pragma once
#include "FSMComponent.h"

class PlayerFSMComponent : public FSMComponent
{
public:
	static constexpr const char* StaticTypeName = "PlayerFSMComponent";
	const char* GetTypeName() const override;

	PlayerFSMComponent();
	virtual ~PlayerFSMComponent() override;

	void Start() override;
	void DispatchEvent(const std::string& eventName);

protected:
	std::optional<std::string> TranslateEvent(EventType type, const void* data) override;
};

