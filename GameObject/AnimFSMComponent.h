#pragma once
#include "FSMComponent.h"

class AnimFSMComponent : public FSMComponent
{
public:
	static constexpr const char* StaticTypeName = "AnimFSMComponent";
	const char* GetTypeName() const override;

	AnimFSMComponent();
	~AnimFSMComponent() override;

	void Start() override;
	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void Notify(const std::string& notifyName);

private:
	bool IsWithinBounds(const POINT& pos) const;
	int ResolveOwnerActorId() const;
	bool m_IsHovered = false;
	bool m_WasAnimPlaying = false;
};

