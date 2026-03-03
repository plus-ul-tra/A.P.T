#pragma once
#include "UIComponent.h"
#include "ResourceHandle.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <array>

class UIManager;
class Scene;
class UIObject;
class EventDispatcher;
		
class InitiativeUIComponent : public UIComponent
{
public:
	static constexpr const char* StaticTypeName = "InitiativeUIComponent";
	const char* GetTypeName() const override;
	~InitiativeUIComponent() override;

	void Start  () override;
	void Update (float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void		SetEnabled(const bool& enable);
	const bool& GetEnabled() const		{ return m_Enabled;   }

	void		 SetScale(const float& scale);
	const float& GetScale() const { return m_Scale; }
	void				 SetDeadIconTexture(const TextureHandle& handle)   { m_DeadIconTexture = handle;   }
	const TextureHandle& GetDeadIconTexture() const						   { return m_DeadIconTexture;	   }

	void RebuildUI();
	void RemoveUI();
	void DetachFromDispatcher();

private:
	struct ActorIconInfo
	{
		bool isPlayer	  = false;
		int  displayIndex = 0;
		int  enemyType = 0;
		int  typeIndex = 0;
	};

	void	   RebuildInitiativeOrder();
	bool       TryPrepareRuntimeBindings();
	std::shared_ptr<UIObject> FindUI(UIManager& uiManager, Scene& scene, const std::string& name) const;
	ActorIconInfo ResolveActorInfo(int actorId) const;
	void ResetIconPools();
	void UpdateIconVisuals(UIObject& iconObject, int actorId, const ActorIconInfo& info) const;
	void UpdateActiveSlotState();
	void SetFrameVisible(bool visible);
	bool IsActorDead(int actorId) const;
	int  GetEnemyTypeForActorId(int actorId) const;

	bool  m_Enabled  = false;
	bool  m_InCombat = false;
	float m_Scale = 1.25f;
	int   m_CurrentActorId = 0;
	bool  m_OrderDirty = false;

	std::vector<int> m_InitiativeOrder;
	std::vector<std::shared_ptr<UIObject>> m_PlayerIconPool;
	std::array<std::vector<std::shared_ptr<UIObject>>, 3> m_EnemyTypeIconPools;
	std::unordered_map<int, std::shared_ptr<UIObject>> m_ActorIcons;
	std::unordered_map<int, ActorIconInfo> m_ActorInfo;
	static constexpr const char* kPlayerIconPrefix = "InitiativePlayerIcon_";
	static constexpr const char* kEnemyType1IconPrefix = "InitiativeEnemyType1Icon_";
	static constexpr const char* kEnemyType2IconPrefix = "InitiativeEnemyType2Icon_";
	static constexpr const char* kEnemyType3IconPrefix = "InitiativeEnemyType3Icon_";

	TextureHandle m_DeadIconTexture   = TextureHandle::Invalid();
	EventDispatcher* m_Dispatcher = nullptr;
	bool m_RuntimeBindingsReady = false;
	bool m_ListenersRegistered = false;
};

