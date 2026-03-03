#pragma once
#include "FSMComponent.h"

class PlayerInventoryFSMComponent : public FSMComponent
{
public:
	static constexpr const char* StaticTypeName = "PlayerInventoryFSMComponent";
	const char* GetTypeName() const override;

	PlayerInventoryFSMComponent();
	virtual ~PlayerInventoryFSMComponent() override = default;

	void Start() override;
};