#pragma once
#include "Component.h"
#include "IEventListener.h"

class NodeComponent;
class AnimFSMComponent;
class AnimationComponent;

class DoorComponent : public Component, public IEventListener
{
public:
	static constexpr const char* StaticTypeName = "DoorComponent";
	const char* GetTypeName() const override;

	DoorComponent();
	virtual ~DoorComponent();

	void Start() override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void OpenDoor();
	bool IsOpen() const { return m_IsOpen; }

	void SetLinkedNodeName(const std::string& name) { m_LinkedNodeName = name; }
	const std::string& GetLinkedNodeName() const { return m_LinkedNodeName; }


private :
	void Resolve();
	NodeComponent*       m_Node = nullptr;
	AnimFSMComponent*    m_AnimFsm = nullptr;
	AnimationComponent*  m_Animation = nullptr;
	bool m_IsOpen = false;
	std::string m_LinkedNodeName;
};

