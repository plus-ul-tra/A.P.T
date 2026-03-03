#include "Component.h"
#include "CameraComponent.h"
#include "TransformComponent.h"
#include "DirectionalLightComponent.h"
#include "PointLightComponent.h"
#include "SpotLightComponent.h"
#include "MaterialComponent.h"
#include "MeshComponent.h"
#include "MeshRenderer.h"
#include "UIComponent.h"
#include "AnimationComponent.h"
#include "SkinningAnimationComponent.h"
#include "SkeletalMeshComponent.h"
#include "SkeletalMeshRenderer.h"
#include "FSMComponent.h"
#include "CollisionFSMComponent.h"
#include "AnimFSMComponent.h"
#include "UIFSMComponent.h"
#include "UIButtonComponent.h"
#include "UITextComponent.h"
#include "UIProgressBarComponent.h"
#include "UISliderComponent.h"
#include "SizeBox.h"
#include "ScaleBox.h"
#include "Border.h"
#include "Canvas.h"
#include "HorizontalBox.h"
#include "UIImageComponent.h"
#include "BoxColliderComponent.h"
#include "SceneChangeTestComponent.h"
#include "PlayerMovementComponent.h"
#include "GridSystemComponent.h"
#include "NodeComponent.h"
#include "PlayerComponent.h"
#include "EnemyComponent.h"
#include "InputEventTestComponent.h"
#include "StatComponent.h"
#include "PlayerStatComponent.h"
#include "EnemyStatComponent.h"
#include "ItemComponent.h"
#include "ItemSpawnerComponent.h"
#include "PlayerFSMComponent.h"
#include "PlayerMoveFSMComponent.h"
#include "PlayerShopFSMComponent.h"
#include "PlayerPushFSMComponent.h"
#include "PlayerCombatFSMComponent.h"
#include "PlayerInventoryFSMComponent.h"
#include "PlayerDoorFSMComponent.h"
#include "FloodSystemComponent.h"
#include "FloodUIComponent.h"
#include "InitiativeUIComponent.h"
#include "PlayerVisualPresetComponent.h"
#include "UIDiceDisplayComponent.h"
#include "UIDiceRollAnimationComponent.h"
#include "UIDicePanelComponent.h"
#include "WaterRiseComponent.h"

// 중앙 등록 .cpp
// exe에서 .lib의 obj를 가져오기 위해 심볼을 연결하기 위한 것
// 비워도 됨 

// 이름 Mangling 제거 "C"
extern "C" {

	void Link_TransformComponent();
	void Link_MeshRenderer();
	void Link_CameraComponent();
	void Link_MaterialComponent();
	void Link_MeshComponent();
	void Link_DirectionalLightComponent();
	void Link_PointLightComponent();
	void Link_SpotLightComponent();
	void Link_UIComponent();
	void Link_AnimationComponent();
	void Link_SkinningAnimationComponent();
	void Link_SkeletalMeshComponent();
	void Link_SkeletalMeshRenderer();
	void Link_FSMComponent();
	void Link_CollisionFSMComponent();
	void Link_AnimFSMComponent();
	void Link_UIFSMComponent();
	void Link_UIButtonComponent();
	void Link_UITextComponent();
	void Link_UIProgressBarComponent();
	void Link_UISliderComponent();
	void Link_SizeBox();
	void Link_ScaleBox();
	void Link_Border();
	void Link_Canvas();
	void Link_HorizontalBox();
	void Link_UIImageComponent();
	void Link_BoxColliderComponent();

	//User Defined
	void Link_SceneChangeTestComponent();
	void Link_PlayerMovementComponent();
	void Link_EnemyMovementComponent();
	void Link_GridSystemComponent();
	void Link_NodeComponent();
	void Link_PlayerComponent();
	void Link_EnemyComponent();
	void Link_InputEventTestComponent();
	void Link_StatComponent();
	void Link_PlayerStatComponent();
	void Link_EnemyStatComponent();
	void Link_EnemyControllerComponent();
	void Link_PlayerFSMComponent();
	void Link_PlayerMoveFSMComponent();
	void Link_PlayerShopFSMComponent();
	void Link_PlayerPushFSMComponent();
	void Link_PlayerCombatFSMComponent();
	void Link_PlayerInventoryFSMComponent();
	void Link_PlayerDoorFSMComponent();
	void Link_ItemComponent();
	void Link_CameraLogicComponent();
	void Link_CameraLogicComponent2();
	void Link_FloodSystemComponent();
	void Link_FloodUIComponent();
	void Link_PushNodeComponent();
	void Link_ItemSpawnerComponent();
	void Link_InitiativeUIComponent();
	void Link_PlayerVisualPresetComponent();
	void Link_UIDiceDisplayComponent();
	void Link_UIDiceRollAnimationComponent();
	void Link_UIDicePanelComponent();
	void Link_BoatComponent();
	void Link_BoatEndComponent();
	void Link_VendingComponent();
	void Link_ExitComponent();
	void Link_WaterRiseComponent();
	void Link_SceneDelayComponent();

}


void RegisterUIFSMDefinitions();
void RegisterCollisionFSMDefinitions();
void RegisterAnimFSMDefinitions();
void RegisterFSMBaseDefinitions();
void RegisterPlayerFSMDefinitions();

//해당 함수는 client.exe에서 한번 호출로 component들의 obj 를 가져올 명분제공
void LinkEngineComponents() {

	Link_TransformComponent();
	Link_MeshRenderer();
	Link_CameraComponent();
	Link_MaterialComponent();
	Link_MeshComponent();
	Link_DirectionalLightComponent();
	Link_PointLightComponent();
	Link_SpotLightComponent();
	Link_UIComponent();
	Link_AnimationComponent();
	Link_SkinningAnimationComponent();
	Link_SkeletalMeshComponent();
	Link_SkeletalMeshRenderer();
	Link_FSMComponent();
	Link_CollisionFSMComponent();
	Link_AnimFSMComponent();
	Link_UIFSMComponent();
	Link_UIButtonComponent();
	Link_UITextComponent();
	Link_UIProgressBarComponent();
	Link_UISliderComponent();
	Link_SizeBox();
	Link_ScaleBox();
	Link_Border();
	Link_Canvas();
	Link_HorizontalBox();
	Link_UIImageComponent();
	Link_SceneChangeTestComponent();
	Link_BoxColliderComponent();


	RegisterFSMBaseDefinitions();
	RegisterUIFSMDefinitions();
	RegisterCollisionFSMDefinitions();
	RegisterAnimFSMDefinitions();
	RegisterPlayerFSMDefinitions();

	//User Defined
	Link_SceneChangeTestComponent();
	Link_PlayerMovementComponent();
	Link_EnemyMovementComponent();
	Link_GridSystemComponent();
	Link_NodeComponent();
	Link_PlayerComponent();
	Link_EnemyComponent();
	Link_InputEventTestComponent();
	Link_StatComponent();
	Link_PlayerStatComponent();
	Link_EnemyStatComponent();
	Link_EnemyControllerComponent();
	Link_PlayerFSMComponent();
	Link_PlayerMoveFSMComponent();
	Link_PlayerShopFSMComponent();
	Link_PlayerPushFSMComponent();
	Link_PlayerCombatFSMComponent();
	Link_PlayerInventoryFSMComponent();
	Link_PlayerDoorFSMComponent();
	Link_ItemComponent();
	Link_CameraLogicComponent();
	Link_CameraLogicComponent2();
	Link_FloodSystemComponent();
	Link_FloodUIComponent();
	Link_PushNodeComponent();
	Link_ItemSpawnerComponent();
	Link_InitiativeUIComponent();
	Link_PlayerVisualPresetComponent();
	Link_UIDiceDisplayComponent();
	Link_UIDiceRollAnimationComponent();
	Link_UIDicePanelComponent();
	Link_BoatComponent();
	Link_BoatEndComponent();
	Link_VendingComponent();
	Link_ExitComponent();
	Link_WaterRiseComponent();
	Link_SceneDelayComponent();
}