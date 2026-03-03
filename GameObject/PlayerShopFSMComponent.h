#pragma once
#include "FSMComponent.h"

class PlayerShopFSMComponent : public FSMComponent
{
public:
	static constexpr const char* StaticTypeName = "PlayerShopFSMComponent";
	const char* GetTypeName() const override;

	PlayerShopFSMComponent();
	virtual ~PlayerShopFSMComponent() override = default;

	void Start() override;

private:
	int m_SelectedPrice = 0;
	int m_SelectedItemId = -1;
};