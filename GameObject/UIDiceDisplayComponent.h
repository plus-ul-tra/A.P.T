#pragma once
#include "UIComponent.h"
#include "UIDiceDisplayTypes.h"
#include "ResourceHandle.h"
#include <array>
#include <string>
#include <vector>

class EventDispatcher;
class UIManager;
class UIObject;
class Scene;
namespace Events
{
	struct DiceRollEvent;
	struct DiceStatResolvedEvent;
}

class UIDiceDisplayComponent : public UIComponent
{
public:
	static constexpr const char* StaticTypeName = "UIDiceDisplayComponent";
	~UIDiceDisplayComponent() override;
	const char* GetTypeName() const override;

	void Start() override;
	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void		SetEnabled(const bool& enabled);
	const bool& GetEnabled() const { return m_Enabled; }

	void			  SetDiceType(const std::string& type);
	const std::string& GetDiceType() const { return m_DiceType; }

	void	   SetValue(const int& value);
	const int& GetValue() const { return m_Value; }
	void SetValueFromRollFace(const int& face);
	void SetValueFromRollFaces(const std::vector<int>& faces, int index);

	void		SetLeadingZero(const bool& leadingZero);
	const bool& GetLeadingZero() const { return m_LeadingZero; }

	void			   SetDiceContext(const std::string& context);
	const std::string& GetDiceContext() const { return m_DiceContext; }

	void		SetAutoShow(const bool& autoShow);
	const bool& GetAutoShow() const { return m_AutoShow; }

	void		SetUseSidesForType(const bool& useSidesForType);
	const bool& GetUseSidesForType() const { return m_UseSidesForType; }

	void			   SetTensDigitObjectName(const std::string& name);
	const std::string& GetTensDigitObjectName() const { return m_TensDigitObjectName; }

	void			   SetOnesDigitObjectName(const std::string& name);
	const std::string& GetOnesDigitObjectName() const { return m_OnesDigitObjectName; }

	void				 SetDigitTextureHandle(int digit, const TextureHandle& handle);
	const TextureHandle& GetDigitTextureHandle(int digit) const;

	void RefreshVisuals();

	void SetDigitTextures(const std::array<TextureHandle, 10>& textures);
	const std::array<TextureHandle, 10>& GetDigitTextures() const { return m_DigitTextures; }

	void SetLayouts(std::vector<UIDiceLayout> layouts);
	const std::vector<UIDiceLayout>& GetLayouts() const { return m_Layouts; }

	void		SetShowTotals(const bool& show);
	const bool& GetShowTotals() const { return m_ShowTotals; }

	void		SetShowIndividuals(const bool& show);
	const bool& GetShowIndividuals() const { return m_ShowIndividuals; }

	void		SetUseRollFaces(const bool& useFaces);
	const bool& GetUseRollFaces() const { return m_UseRollFaces; }

	void	   SetRollIndex(const int& index);
	const int& GetRollIndex() const { return m_RollIndex; }

private:
	UIObject* FindUIObject(const std::string& name) const;
	const UIDiceLayout* FindLayout() const;
	void ApplyLayout(const UIDiceLayout& layout, UIObject& owner, UIObject* tens, UIObject* ones);
	void ApplyValue(UIObject* tens, UIObject* ones);
	void ApplyDiceEvent(const Events::DiceRollEvent& payload);
	void ApplyDiceStatResolvedEvent(const Events::DiceStatResolvedEvent& payload);
	void ApplyDisabledVisibility();
	bool TryPrepareRuntimeBindings();
	static int ResolveNumericSuffix(const std::string& context);
	static std::string ResolveBaseContext(const std::string& context);


	std::array<TextureHandle, 10> m_DigitTextures{};
	std::vector<UIDiceLayout> m_Layouts;
	std::string m_DiceType;
	std::string m_DiceContext;
	std::string m_TensDigitObjectName = "DiceTens";
	std::string m_OnesDigitObjectName = "DiceOnes";

	int  m_Value = 0;
	bool m_LeadingZero = true;
	bool m_Enabled = true;
	bool m_LayoutDirty = true;
	bool m_ValueDirty = true;
	bool m_AutoShow = true;
	bool m_UseSidesForType = true;
	bool m_ShowTotals = true;
	bool m_ShowIndividuals = true;
	bool m_UseRollFaces = false;
	bool m_HasLayout = false;
	int  m_RollIndex = 0;
	UIDiceDigitSlot m_TensSlot{};
	UIDiceDigitSlot m_OnesSlot{};
	EventDispatcher* m_Dispatcher = nullptr;
	bool m_RuntimeBindingsReady = false;
	bool m_ListenersRegistered = false;
	bool m_DisabledVisibilityPending = false;
};

