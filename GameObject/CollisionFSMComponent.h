#pragma once
#include "FSMComponent.h"

class CollisionFSMComponent : public FSMComponent
{
public:
	static constexpr const char* StaticTypeName = "CollisionFSMComponent";
	const char* GetTypeName() const override;

	CollisionFSMComponent();
	~CollisionFSMComponent() override;

	void Start() override;

protected:
	std::optional<std::string> TranslateEvent(EventType type, const void* data) override;
};

