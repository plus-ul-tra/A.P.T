#include "AnimFSMComponent.h"
#include "AnimBlendCurveFunction.h"
#include "AnimationComponent.h"
#include "FSMActionRegistry.h"
#include "FSMEventRegistry.h"
#include "Object.h"
#include "ReflectionMacro.h"
#include "UIObject.h"
#include "BoxColliderComponent.h"
#include "Scene.h"
#include "GameObject.h"
#include "CameraObject.h"
#include "TransformComponent.h"
#include "CameraComponent.h"
#include "RayHelper.h"
#include "Event.h"
#include "EnemyComponent.h"
#include "PlayerComponent.h"

AnimationComponent::BlendCurveFn ResolveBlendCurve(const std::string& name)
{
	if (name == "EaseInSine") return BlendCurveFunc::EaseInSine;
	if (name == "EaseOutSine") return BlendCurveFunc::EaseOutSine;
	if (name == "EaseInOutSine") return BlendCurveFunc::EaseInOutSine;
	if (name == "EaseInQuad") return BlendCurveFunc::EaseInQuad;
	if (name == "EaseOutQuad") return BlendCurveFunc::EaseOutQuad;
	if (name == "EaseInOutQuad") return BlendCurveFunc::EaseInOutQuad;
	if (name == "EaseInCubic") return BlendCurveFunc::EaseInCubic;
	if (name == "EaseOutCubic") return BlendCurveFunc::EaseOutCubic;
	if (name == "EaseInOutCubic") return BlendCurveFunc::EaseInOutCubic;
	if (name == "EaseInQuart") return BlendCurveFunc::EaseInQuart;
	if (name == "EaseOutQuart") return BlendCurveFunc::EaseOutQuart;
	if (name == "EaseInOutQuart") return BlendCurveFunc::EaseInOutQuart;
	if (name == "EaseInQuint") return BlendCurveFunc::EaseInQuint;
	if (name == "EaseOutQuint") return BlendCurveFunc::EaseOutQuint;
	if (name == "EaseInOutQuint") return BlendCurveFunc::EaseInOutQuint;
	if (name == "EaseInExpo") return BlendCurveFunc::EaseInExpo;
	if (name == "EaseOutExpo") return BlendCurveFunc::EaseOutExpo;
	if (name == "EaseInOutExpo") return BlendCurveFunc::EaseInOutExpo;
	if (name == "EaseInCirc") return BlendCurveFunc::EaseInCirc;
	if (name == "EaseOutCirc") return BlendCurveFunc::EaseOutCirc;
	if (name == "EaseInOutCirc") return BlendCurveFunc::EaseInOutCirc;
	if (name == "EaseInBack") return BlendCurveFunc::EaseInBack;
	if (name == "EaseOutBack") return BlendCurveFunc::EaseOutBack;
	if (name == "EaseInOutBack") return BlendCurveFunc::EaseInOutBack;
	if (name == "EaseInElastic") return BlendCurveFunc::EaseInElastic;
	if (name == "EaseOutElastic") return BlendCurveFunc::EaseOutElastic;
	if (name == "EaseInElastic") return BlendCurveFunc::EaseInOutElastic;
	if (name == "EaseInBounce") return BlendCurveFunc::EaseInBounce;
	if (name == "EaseOutBounce") return BlendCurveFunc::EaseOutBounce;
	if (name == "EaseInOutBounce") return BlendCurveFunc::EaseInOutBounce;
	
	
	return nullptr;
}

void RegisterAnimFSMDefinitions()
{
	auto& actionRegistry = FSMActionRegistry::Instance();
	actionRegistry.RegisterAction({
		"Anim_Play",
		"Animation",
		{}
		});
	actionRegistry.RegisterAction({
		"Anim_Stop",
		"Animation",
		{}
		});
	actionRegistry.RegisterAction({
		"Anim_SetSpeed",
		"Animation",
		{
			{ "value", "float", 1.0f, false }
		}
		});

	actionRegistry.RegisterAction({
		"Anim_BlendTo",
		"Animation",
		{
			{ "assetPath", "string", "", true },
			{ "assetIndex", "int", 0, false },
			{ "blendTime", "float", 0.2f, false },
			{ "curve", "string", "Linear", false }
		}
		});

	auto& eventRegistry = FSMEventRegistry::Instance();
	eventRegistry.RegisterEvent({ "AnimNotify_Hit", "Animation" });
	eventRegistry.RegisterEvent({ "AnimNotify_Footstep", "Animation" });
	eventRegistry.RegisterEvent({ "Anim_Play", "Animation" });
	eventRegistry.RegisterEvent({ "Anim_TriggerMoveComplete", "Animation" });
	eventRegistry.RegisterEvent({ "Anim_TriggerPlayerAttack", "Animation" });
	eventRegistry.RegisterEvent({ "Anim_TriggerEnemyAttack", "Animation" });
	eventRegistry.RegisterEvent({ "Anim_TriggerClick", "Animation" });
	eventRegistry.RegisterEvent({ "Anim_Finished", "Animation" });
}

REGISTER_COMPONENT_DERIVED(AnimFSMComponent, FSMComponent)

AnimFSMComponent::AnimFSMComponent()
{
	BindActionHandler("Anim_Play", [this](const FSMAction&)
		{
			auto* owner = GetOwner();
			auto anims = owner ? owner->GetComponentsDerived<AnimationComponent>() : std::vector<AnimationComponent*>{};
			auto* anim = anims.empty() ? nullptr : anims.front();
			if (anim)
			{
				anim->SeekTime(0.0f);
				anim->Play();
				m_WasAnimPlaying = true;
			}
		});

	BindActionHandler("Anim_Stop", [this](const FSMAction&)
		{
			auto* owner = GetOwner();
			auto anims = owner ? owner->GetComponentsDerived<AnimationComponent>() : std::vector<AnimationComponent*>{};
			auto* anim = anims.empty() ? nullptr : anims.front();
			if (anim)
			{
				anim->Stop();
			}
		});

	BindActionHandler("Anim_SetSpeed", [this](const FSMAction& action)
		{
			auto* owner = GetOwner();
			auto anims = owner ? owner->GetComponentsDerived<AnimationComponent>() : std::vector<AnimationComponent*>{};
			auto* anim = anims.empty() ? nullptr : anims.front();
			if (!anim)
				return;

			auto playback = anim->GetPlayback();
			playback.speed = action.params.value("value", playback.speed);
			anim->SetPlayback(playback);
		});

	BindActionHandler("Anim_BlendTo", [this](const FSMAction& action)
		{
			auto* owner = GetOwner();
			auto anims = owner ? owner->GetComponentsDerived<AnimationComponent>() : std::vector<AnimationComponent*>{};
			auto* anim = anims.empty() ? nullptr : anims.front();
			if (!anim)
				return;

			const int handleId = action.params.value("handleId", 0);
			const int handleGen = action.params.value("handleGen", 0);
			const std::string assetPath = action.params.value("assetPath", std::string{});
			const int assetIndex = action.params.value("assetIndex", 0);
			const float blendTime = action.params.value("blendTime", 0.2f);
			const std::string curveName = action.params.value("curve", std::string("Linear"));
			const bool useBlendConfig = action.params.value("useBlendConfig", false);

			AnimationHandle handle = AnimationHandle::Invalid();
			float resolvedBlendTime = blendTime;
			std::string resolvedCurve = curveName;
			AnimationComponent::BlendType resolvedType = AnimationComponent::BlendType::Linear;

			if (useBlendConfig)
			{
				const auto& config = anim->GetBlendConfig();
				handle = config.toClip;
				resolvedBlendTime = config.blendTime;
				resolvedCurve = config.curveName;
				resolvedType = config.blendType;
			}
			else if (handleId > 0)
			{
				handle.id = static_cast<UINT32>(handleId);
				handle.generation = static_cast<UINT32>(handleGen);
			}
			else if (!assetPath.empty())
			{
				auto* loader = AssetLoader::GetActive();
				if (!loader)
					return;

				handle = loader->ResolveAnimation(assetPath, static_cast<UINT32>(assetIndex));
			}

			if (!handle.IsValid())
				return;

			if (resolvedCurve == "Linear" && resolvedType != AnimationComponent::BlendType::Curve)
			{
				anim->StartBlend(handle, resolvedBlendTime);
				return;
			}

			if (auto curveFn = ResolveBlendCurve(resolvedCurve))
			{
				anim->StartBlend(handle, resolvedBlendTime, AnimationComponent::BlendType::Curve, curveFn);
				return;
			}

			anim->StartBlend(handle, resolvedBlendTime);
		});
}


AnimFSMComponent::~AnimFSMComponent()
{
	//GetEventDispatcher().RemoveListener(EventType::UIHovered, this);
	GetEventDispatcher().RemoveListener(EventType::MouseLeftClick, this);
	GetEventDispatcher().RemoveListener(EventType::PlayerMove, this);
	GetEventDispatcher().RemoveListener(EventType::PlayerAttack, this);
	GetEventDispatcher().RemoveListener(EventType::EnemyAttack, this);
}

void AnimFSMComponent::Start()
{
	FSMComponent::Start();
	//GetEventDispatcher().AddListener(EventType::UIHovered, this);
	GetEventDispatcher().AddListener(EventType::MouseLeftClick, this);
	GetEventDispatcher().AddListener(EventType::PlayerMove, this);
	GetEventDispatcher().AddListener(EventType::PlayerAttack, this);
	GetEventDispatcher().AddListener(EventType::EnemyAttack, this);
}

void AnimFSMComponent::Update(float deltaTime)
{
	FSMComponent::Update(deltaTime);

	auto* owner = GetOwner();
	auto anims = owner ? owner->GetComponentsDerived<AnimationComponent>() : std::vector<AnimationComponent*>{};
	auto* anim = anims.empty() ? nullptr : anims.front();
	if (!anim)
	{
		m_WasAnimPlaying = false;
		return;
	}

	const bool isPlaying = anim->GetPlayback().playing;
	if (m_WasAnimPlaying && !isPlaying)
	{
		DispatchEvent("Anim_Finished");
	}
	m_WasAnimPlaying = isPlaying;
}

void AnimFSMComponent::OnEvent(EventType type, const void* data)
{
	//if (type == EventType::UIHovered)
	//{
	//	const auto* mouseData = static_cast<const Events::MouseState*>(data);
	//	if (!mouseData)
	//	{
	//		return;
	//	}

	//	if (GetOwner() && !GetOwner()->GetComponent<PlayerComponent>())
	//	{
	//		return;
	//	}

	//	const bool isHovered = IsWithinBounds(mouseData->pos);
	//	if (isHovered && !m_IsHovered)
	//	{
	//		DispatchEvent("Anim_TriggerHover");
	//		DispatchEvent("Anim_Play");
	//	}
	//	m_IsHovered = isHovered;
	//	return;
	//}

	if (type == EventType::MouseLeftClick)
	{
		const auto* mouseData = static_cast<const Events::MouseState*>(data);
		if (!mouseData)
		{
			return;
		}

		auto* owner = GetOwner();
		if (!owner || !owner->GetComponent<PlayerComponent>())
		{
			return;
		}

		if (!IsWithinBounds(mouseData->pos))
		{
			return;
		}

		DispatchEvent("Anim_TriggerClick");
		DispatchEvent("Anim_Play");
		return;
	}

	if (type == EventType::PlayerMove || type == EventType::PlayerAttack || type == EventType::EnemyAttack)
	{
		const auto* actorEvent = static_cast<const Events::ActorEvent*>(data);
		if (!actorEvent)
		{
			return;
		}

		const int ownerActorId = ResolveOwnerActorId();
		if (ownerActorId == 0 || ownerActorId != actorEvent->actorId)
		{
			return;
		}

		if (type == EventType::PlayerMove)
		{
			DispatchEvent("Anim_TriggerMoveComplete");
		}
		else if (type == EventType::PlayerAttack)
		{
			DispatchEvent("Anim_TriggerPlayerAttack");
		}
		else
		{
			DispatchEvent("Anim_TriggerEnemyAttack");
		}

		DispatchEvent("Anim_Play");
		return;
	}

	FSMComponent::OnEvent(type, data);
}

int AnimFSMComponent::ResolveOwnerActorId() const
{
	auto* owner = GetOwner();
	if (!owner)
	{
		return 0;
	}

	if (auto* player = owner->GetComponent<PlayerComponent>())
	{
		return player->GetActorId();
	}

	if (auto* enemy = owner->GetComponent<EnemyComponent>())
	{
		return enemy->GetActorId();
	}

	return 0;
}

bool AnimFSMComponent::IsWithinBounds(const POINT& pos) const
{
	if (auto* uiObject = dynamic_cast<UIObject*>(GetOwner()))
	{
		return uiObject->HitCheck(pos);
	}

	auto* owner = GetOwner();
	if (!owner)
	{
		return false;
	}

	if (auto* collider = owner->GetComponent<BoxColliderComponent>())
	{
		auto* scene = owner->GetScene();
		if (!scene)
		{
			return false;
		}

		auto cameraObject = scene->GetGameCamera();
		if (!cameraObject)
		{
			cameraObject = scene->GetEditorCamera();
		}

		if (!cameraObject)
		{
			return false;
		}

		auto* camera = cameraObject->GetComponent<CameraComponent>();
		if (!camera)
		{
			return false;
		}

		const auto view = camera->GetViewMatrix();
		const auto proj = camera->GetProjMatrix();
		const auto& viewport = camera->GetViewport();
		if (viewport.Width <= 0.0f || viewport.Height <= 0.0f)
		{
			return false;
		}

		const auto viewMat = DirectX::XMLoadFloat4x4(&view);
		const auto projMat = DirectX::XMLoadFloat4x4(&proj);
		const Ray ray = MakePickRayLH(
			static_cast<float>(pos.x),
			static_cast<float>(pos.y),
			0.0f,
			0.0f,
			viewport.Width,
			viewport.Height,
			viewMat,
			projMat);

		float hitT = 0.0f;
		return collider->IntersectsRay(ray.m_Pos, ray.m_Dir, hitT);
	}

	return false;
}

void AnimFSMComponent::Notify(const std::string& notifyName)
{
	DispatchEvent("AnimNotify_" + notifyName);
}
