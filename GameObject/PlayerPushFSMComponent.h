#pragma once
#include "FSMComponent.h"

class PlayerPushFSMComponent : public FSMComponent
{
public:
	static constexpr const char* StaticTypeName = "PlayerPushFSMComponent";
	const char* GetTypeName() const override;

	PlayerPushFSMComponent();
	virtual ~PlayerPushFSMComponent() override = default;

	void Start() override;
	void Update(float deltaTime) override;
};