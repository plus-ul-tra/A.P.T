#pragma once
#include "FSMComponent.h"

class PlayerDoorFSMComponent : public FSMComponent
{
public:
	static constexpr const char* StaticTypeName = "PlayerDoorFSMComponent";
	const char* GetTypeName() const override;

	PlayerDoorFSMComponent();
	virtual ~PlayerDoorFSMComponent() override = default;

	void Start() override;
	void Update(float deltaTime) override;


};