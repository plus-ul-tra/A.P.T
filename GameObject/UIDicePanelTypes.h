#pragma once
#include <string>

struct UIDicePanelSlot
{
	std::string objectName;
	std::string diceType;
	std::string diceContext;
	int requiredDiceCount = 0;
	bool applyAnimation = true;
};