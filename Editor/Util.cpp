// Game / Client
#include "pch.h"
#include "EditorApplication.h"
#include "CameraObject.h"
#include "CameraComponent.h"
#include "MeshComponent.h"
#include "MeshRenderer.h"
#include "MaterialComponent.h"
#include "SkeletalMeshComponent.h"
#include "SkeletalMeshRenderer.h"
#include "AnimationComponent.h"
#include "TransformComponent.h"
#include "LightComponent.h"
#include "GameObject.h"
#include "Reflection.h"
#include "Renderer.h"
#include "Scene.h"
#include "DX11.h"
#include "Util.h"
#include "json.hpp"
#include "FSMActionRegistry.h"
#include "FSMComponent.h"
#include "FSMEventRegistry.h"
#include "UIPrimitives.h"
#include "HorizontalBox.h"
#include "EnemyMovementComponent.h"
#include "Canvas.h"
#include "UIDiceDisplayTypes.h"
#include "UIDicePanelTypes.h"

#define DRAG_SPEED 0.01f
bool SceneHasObjectName(const Scene& scene, const std::string& name)
{
	const auto& gameObjects = scene.GetGameObjects();
	return gameObjects.find(name) != gameObjects.end();
}

// 이름 변경용 helper 
void CopyStringToBuffer(const std::string& value, std::array<char, 256>& buffer)
{
	std::snprintf(buffer.data(), buffer.size(), "%s", value.c_str());
}

XMFLOAT3 QuaternionToEulerRadians(const XMFLOAT4& quaternion)
{
	const XMVECTOR qNormalized = XMQuaternionNormalize(XMLoadFloat4(&quaternion));
	XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(qNormalized);
	XMFLOAT4X4 m{};
	XMStoreFloat4x4(&m, rotationMatrix);

	const float sinPitch = -m._32;
	const float pitch = std::asin(std::clamp(sinPitch, -1.0f, 1.0f));
	const float cosPitch = std::cos(pitch);

	float roll = 0.0f;
	float yaw = 0.0f;

	if (std::abs(cosPitch) > 1e-4f)
	{
		roll = std::atan2(m._12, m._22);
		yaw = std::atan2(m._31, m._33);
	}
	else
	{
		roll = std::atan2(-m._21, m._11);
		yaw = 0.0f;
	}

	return { pitch, yaw, roll };
}

XMFLOAT4 EulerRadiansToQuaternion(const XMFLOAT3& eulerRadians)
{
	XMFLOAT4 result{};
	XMStoreFloat4(&result, XMQuaternionRotationRollPitchYaw(eulerRadians.x, eulerRadians.y, eulerRadians.z));
	return result;
}

// 이름 중복방지(넘버링)
std::string MakeUniqueObjectName(const Scene& scene, const std::string& baseName)
{
	if (!SceneHasObjectName(scene, baseName))
	{
		return baseName;
	}
	for (int index = 1; index < 10000; ++index)
	{
		std::string candidate = baseName + std::to_string(index);
		if (!SceneHasObjectName(scene, candidate))
		{
			return candidate;
		}
	}

	return baseName + "_Overflow"; // 같은 이름이 10000개 넘을 리는 없을 것으로 생각. 일단 막아둠  <- 기본 리턴값 없어서 다시 주석 해제함
}

float NormalizeDegrees(float degrees)
{
	float normalized = std::fmod(degrees, 360.0f);
	if (normalized > 180.0f)
	{
		normalized -= 360.0f;
	}
	else if (normalized < -180.0f)
	{
		normalized += 360.0f;
	}
	return normalized;
}

bool DrawSubMeshOverridesEditor(MeshComponent& meshComponent, AssetLoader& assetLoader)
{
	const MeshHandle meshHandle = meshComponent.GetMeshHandle();
	if (!meshHandle.IsValid())
	{
		ImGui::TextDisabled("SubMesh Overrides (mesh not set)");
		return false;
	}

	const auto* meshData = assetLoader.GetMeshes().Get(meshHandle);
	if (!meshData)
	{
		ImGui::TextDisabled("SubMesh Overrides (mesh not loaded)");
		return false;
	}

	const size_t subMeshCount = meshData->subMeshes.empty() ? 1 : meshData->subMeshes.size();
	const auto& overrides = meshComponent.GetSubMeshMaterialOverrides();
	bool changed = false;

	// Clear UI 상태(직전 Clear 누른 슬롯만 <None>으로 보이게)
	static MeshComponent* s_lastClearedComp = nullptr;
	static int s_lastClearedIndex = -1;

	auto resolveMaterialDisplay = [&assetLoader](const MaterialHandle& handle) -> std::string
		{
			if (!handle.IsValid())
				return {};

			// displayName 우선
			if (const std::string* displayName = assetLoader.GetMaterials().GetDisplayName(handle))
			{
				if (!displayName->empty())
					return *displayName;
			}

			if (const std::string* key = assetLoader.GetMaterials().GetKey(handle))
			{
				if (!key->empty())
					return *key;
			}

			return {};
		};

	// ★ 오브젝트(MaterialComponent) 쪽 "현재 베이스 머티리얼"을 먼저 잡아둠
	MaterialHandle ownerBaseMaterial = MaterialHandle::Invalid();
	if (const auto* owner = meshComponent.GetOwner())
	{
		if (const auto* materialComponent = owner->GetComponent<MaterialComponent>())
			ownerBaseMaterial = materialComponent->GetMaterialHandle();
	}

	if (ImGui::TreeNode("SubMesh Overrides"))
	{
		for (size_t i = 0; i < subMeshCount; ++i)
		{
			ImGui::PushID(static_cast<int>(i));
			ImGui::AlignTextToFramePadding();

			std::string subMeshName;

			// ★ meshbin에 박힌 서브메시 기본 머티리얼 (fallback용)
			MaterialHandle subMeshDefaultMaterial = MaterialHandle::Invalid();
			if (!meshData->subMeshes.empty() && i < meshData->subMeshes.size())
			{
				subMeshName = meshData->subMeshes[i].name;
				subMeshDefaultMaterial = meshData->subMeshes[i].material;
			}

			// ★ UI에서 보여줄 baseMaterial 우선순위:
			// 1) 오브젝트(MaterialComponent) 현재 값
			// 2) meshbin의 submesh 기본값(임포트 당시)
			MaterialHandle baseMaterial = ownerBaseMaterial.IsValid() ? ownerBaseMaterial : subMeshDefaultMaterial;

			std::string baseDisplay = resolveMaterialDisplay(baseMaterial);

			if (!subMeshName.empty())
				ImGui::Text("%s", subMeshName.c_str());
			else
				ImGui::Text("SubMesh %zu", i);
			ImGui::SameLine();

			// override display 결정
			std::string overrideDisplay; // 비어있으면 override 없음
			if (i < overrides.size())
			{
				const auto& overrideRef = overrides[i].material;
				if (!overrideRef.assetPath.empty())
				{
					const MaterialHandle handle = assetLoader.ResolveMaterial(overrideRef.assetPath, overrideRef.assetIndex);
					if (handle.IsValid())
					{
						// displayName 우선
						if (const std::string* displayName = assetLoader.GetMaterials().GetDisplayName(handle))
						{
							if (!displayName->empty())
								overrideDisplay = *displayName;
						}
						if (overrideDisplay.empty())
						{
							if (const std::string* key = assetLoader.GetMaterials().GetKey(handle))
							{
								if (!key->empty())
									overrideDisplay = *key;
							}
						}
					}

					// resolve 실패/표시이름 없음이면 path라도 보여줌
					if (overrideDisplay.empty())
						overrideDisplay = overrideRef.assetPath;
				}
			}

			// Clear 상태 체크
			bool isClear = (s_lastClearedComp == &meshComponent && s_lastClearedIndex == (int)i);

			// shown 우선순위: overrideDisplay > baseDisplay > <None>
			const char* name =
				(!overrideDisplay.empty()) ? overrideDisplay.c_str() :
				isClear ? "<None>" :
				(!baseDisplay.empty()) ? baseDisplay.c_str() :
				"<None>";

			std::string buttonLabel = std::string(name) + "##SubMeshOverride";
			ImGui::Button(buttonLabel.c_str());

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_MATERIAL"))
				{
					const MaterialHandle dropped = *static_cast<const MaterialHandle*>(payload->Data);
					std::string assetPath;
					UINT32 assetIndex = 0;
					if (assetLoader.GetMaterialAssetReference(dropped, assetPath, assetIndex))
					{
						meshComponent.SetSubMeshMaterialOverride(i, MaterialRef{ assetPath, assetIndex });
						changed = true;

						// 드롭으로 override가 들어오면 Clear 상태 해제
						if (s_lastClearedComp == &meshComponent && s_lastClearedIndex == (int)i)
						{
							s_lastClearedComp = nullptr;
							s_lastClearedIndex = -1;
						}
					}
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::SameLine();
			if (ImGui::Button("Clear"))
			{
				meshComponent.ClearSubMeshMaterialOverride(i);
				changed = true;
					
				s_lastClearedIndex = static_cast<int>(i);
				s_lastClearedComp = &meshComponent;
			}

			const auto* overrideData = (i < overrides.size()) ? &overrides[i] : nullptr;
			ShaderAssetHandle shaderAssetHandle = overrideData ? overrideData->shaderAsset : ShaderAssetHandle::Invalid();
			VertexShaderHandle vsHandle = overrideData ? overrideData->vertexShader : VertexShaderHandle::Invalid();
			PixelShaderHandle psHandle = overrideData ? overrideData->pixelShader : PixelShaderHandle::Invalid();

			auto resolveShaderAssetDisplay = [&assetLoader](const ShaderAssetHandle& handle) -> std::string
				{
					if (!handle.IsValid())
						return {};

					if (const std::string* displayName = assetLoader.GetShaderAssets().GetDisplayName(handle))
					{
						if (!displayName->empty())
							return *displayName;
					}

					if (const std::string* key = assetLoader.GetShaderAssets().GetKey(handle))
					{
						if (!key->empty())
							return *key;
					}

					return {};
				};


			auto resolveVertexShaderDisplay = [&assetLoader](const VertexShaderHandle& handle) -> std::string
				{
					if (!handle.IsValid())
						return {};

					if (const std::string* displayName = assetLoader.GetVertexShaders().GetDisplayName(handle))
					{
						if (!displayName->empty())
							return *displayName;
					}

					if (const std::string* key = assetLoader.GetVertexShaders().GetKey(handle))
					{
						if (!key->empty())
							return *key;
					}

					return {};
				};

			auto resolvePixelShaderDisplay = [&assetLoader](const PixelShaderHandle& handle) -> std::string
				{
					if (!handle.IsValid())
						return {};

					if (const std::string* displayName = assetLoader.GetPixelShaders().GetDisplayName(handle))
					{
						if (!displayName->empty())
							return *displayName;
					}

					if (const std::string* key = assetLoader.GetPixelShaders().GetKey(handle))
					{
						if (!key->empty())
							return *key;
					}

					return {};
				};

			std::string vsDisplay = resolveVertexShaderDisplay(vsHandle);
			std::string psDisplay = resolvePixelShaderDisplay(psHandle);
			std::string shaderAssetDisplay = resolveShaderAssetDisplay(shaderAssetHandle);

			const char* vsName = vsDisplay.empty() ? "<None>" : vsDisplay.c_str();
			const char* psName = psDisplay.empty() ? "<None>" : psDisplay.c_str();
			const char* shaderAssetName = shaderAssetDisplay.empty() ? "<None>" : shaderAssetDisplay.c_str();

			ImGui::Indent();
			{
				std::string vsButtonLabel = std::string(vsName) + "##SubMeshVSOverride";
				ImGui::TextUnformatted("Vertex Shader");
				ImGui::SameLine();
				ImGui::Button(vsButtonLabel.c_str());

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_VERTEX_SHADER"))
					{
						const VertexShaderHandle dropped = *static_cast<const VertexShaderHandle*>(payload->Data);
						meshComponent.SetSubMeshVertexShaderOverride(i, dropped);
						changed = true;
					}
					ImGui::EndDragDropTarget();
				}

				ImGui::SameLine();
				if (ImGui::Button("Clear##SubMeshVS"))
				{
					meshComponent.SetSubMeshVertexShaderOverride(i, VertexShaderHandle::Invalid());
					changed = true;
				}

				std::string psButtonLabel = std::string(psName) + "##SubMeshPSOverride";
				ImGui::TextUnformatted("Pixel Shader");
				ImGui::SameLine();
				ImGui::Button(psButtonLabel.c_str());

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_PIXEL_SHADER"))
					{
						const PixelShaderHandle dropped = *static_cast<const PixelShaderHandle*>(payload->Data);
						meshComponent.SetSubMeshPixelShaderOverride(i, dropped);
						changed = true;
					}
					ImGui::EndDragDropTarget();
				}

				ImGui::SameLine();
				if (ImGui::Button("Clear##SubMeshPS"))
				{
					meshComponent.SetSubMeshPixelShaderOverride(i, PixelShaderHandle::Invalid());
					changed = true;
				}

				std::string shaderAssetLabel = std::string(shaderAssetName) + "##SubMeshShaderAssetOverride";
				ImGui::TextUnformatted("Shader Asset");
				ImGui::SameLine();
				ImGui::Button(shaderAssetLabel.c_str());

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_SHADER_ASSET"))
					{
						const ShaderAssetHandle dropped = *static_cast<const ShaderAssetHandle*>(payload->Data);
						meshComponent.SetSubMeshShaderAssetOverride(i, dropped);
						changed = true;
					}
					ImGui::EndDragDropTarget();
				}

				ImGui::SameLine();
				if (ImGui::Button("Clear##SubMeshShaderAsset"))
				{
					meshComponent.SetSubMeshShaderAssetOverride(i, ShaderAssetHandle::Invalid());
					changed = true;
				}
			}
			ImGui::Unindent();


			ImGui::PopID();
		}
		ImGui::TreePop();
	}

	return changed;
}

bool DrawFSMActionParams(FSMAction& action, const std::vector<std::string>& categories)
{
	bool updated = false;

	const FSMActionDef* definition = nullptr;
	for (const auto& category : categories)
	{
		definition = FSMActionRegistry::Instance().FindAction(category, action.id);
		if (definition)
		{
			break;
		}
	}

	if (!definition)
	{
		ImGui::TextDisabled("No Action Schema Available.");
		return false;
	}

	static constexpr const char* kCurveLabels[] =
	{
		"Linear",
		"EaseInSine",
		"EaseOutSine",
		"EaseInOutSine",
		"EaseInQuad",
		"EaseOutQuad",
		"EaseInOutQuad",
		"EaseInCubic",
		"EaseOutCubic",
		"EaseInOutCubic",
		"EaseInQuart",
		"EaseOutQuart",
		"EaseInOutQuart",
		"EaseInQuint",
		"EaseOutQuint",
		"EaseInOutQuint",
		"EaseInExpo",
		"EaseOutExpo",
		"EaseInOutExpo",
		"EaseInCirc",
		"EaseOutCirc",
		"EaseInOutCirc",
		"EaseInBack",
		"EaseOutBack",
		"EaseInOutBack",
		"EaseInElastic",
		"EaseOutElastic",
		"EaseInElastic",
		"EaseInBounce" ,
		"EaseOutBounce",
		"EaseInOutBounce"
	};

	for (const auto& param : definition->params)
	{
		ImGui::PushID(param.name.c_str());
		const std::string label = param.name + " (" + param.type + ")";

		if (param.type == "bool")
		{
			bool value = action.params.value(param.name, param.defaultValue.get<bool>());
			if (ImGui::Checkbox(label.c_str(), &value))
			{
				action.params[param.name] = value;
				updated = true;
			}
		}
		else if (param.type == "float")
		{
			float value = action.params.value(param.name, param.defaultValue.get<float>());
			if (ImGui::DragFloat(label.c_str(), &value, DRAG_SPEED))
			{
				action.params[param.name] = value;
				updated = true;
			}
		}
		else if (param.type == "int")
		{
			int value = action.params.value(param.name, param.defaultValue.get<int>());
			if (ImGui::DragInt(label.c_str(), &value, DRAG_SPEED))
			{
				action.params[param.name] = value;
				updated = true;
			}
		}
		else if (param.type == "string")
		{
			if (action.id == "Anim_BlendTo" && param.name == "curve")
			{
				std::string value = action.params.value(param.name, param.defaultValue.get<std::string>());
				int curveIndex = 0;
				for (int i = 0; i < IM_ARRAYSIZE(kCurveLabels); ++i)
				{
					if (value == kCurveLabels[i])
					{
						curveIndex = i;
						break;
					}
				}

				if (ImGui::Combo(label.c_str(), &curveIndex, kCurveLabels, IM_ARRAYSIZE(kCurveLabels)))
				{
					action.params[param.name] = std::string(kCurveLabels[curveIndex]);
					updated = true;
				}
			}
			else
			{
				std::string value = action.params.value(param.name, param.defaultValue.get<std::string>());
				std::array<char, 256> buffer{};
				CopyStringToBuffer(value, buffer);
				if (ImGui::InputText(label.c_str(), buffer.data(), buffer.size()))
				{
					action.params[param.name] = std::string(buffer.data());
					updated = true;
				}
			}
		}
		else
		{
			ImGui::TextDisabled("%s (unsupported)", label.c_str());
		}

		ImGui::PopID();
	}

	return updated;
}

namespace
{
	std::string ResolveFSMCategory(const Component& component)
	{
		const std::string typeName = component.GetTypeName();
		if (typeName == "PlayerFSMComponent")
		{
			return "Player";
		}
		if (typeName == "PlayerMoveFSMComponent")
		{
			return "Move";
		}
		if (typeName == "PlayerDoorFSMComponent")
		{
			return "Door";
		}
		if (typeName == "PlayerCombatFSMComponent")
		{
			return "Combat";
		}
		if (typeName == "PlayerInventoryFSMComponent")
		{
			return "Inventory";
		}
		if (typeName == "PlayerShopFSMComponent")
		{
			return "Shop";
		}
		if (typeName == "PlayerPushFSMComponent")
		{
			return "Push";
		}
		if (typeName == "AnimFSMComponent")
		{
			return "Animation";
		}
		if (typeName == "UIFSMComponent")
		{
			return "UI";
		}
		if (typeName == "CollisionFSMComponent")
		{
			return "Collision";
		}

		return "Common";
	}

	std::vector<FSMActionDef> CollectFSMActionDefs(const std::vector<std::string>& categories)
	{
		std::vector<FSMActionDef> results;
		std::unordered_set<std::string> seen;
		auto& registry = FSMActionRegistry::Instance();
		for (const auto& category : categories)
		{
			const auto& actions = registry.GetActions(category);
			for (const auto& action : actions)
			{
				if (seen.insert(action.id).second)
				{
					results.push_back(action);
				}
			}
		}
		return results;
	}

	std::vector<FSMEventDef> CollectFSMEventDefs(const std::vector<std::string>& categories)
	{
		std::vector<FSMEventDef> results;
		std::unordered_set<std::string> seen;
		auto& registry = FSMEventRegistry::Instance();
		for (const auto& category : categories)
		{
			const auto& events = registry.GetEvents(category);
			for (const auto& eventDef : events)
			{
				if (seen.insert(eventDef.name).second)
				{
					results.push_back(eventDef);
				}
			}
		}
		return results;
	}
}

bool DrawFSMActionList(const char* label, std::vector<FSMAction>& actions, const std::vector<std::string>& categories)
{
	bool updated = false;
	if (!ImGui::TreeNode(label))
	{
		return false;
	}

	const auto& actionDefs = CollectFSMActionDefs(categories);

	for (size_t i = 0; i < actions.size(); ++i)
	{
		ImGui::PushID(static_cast<int>(i));
		ImGui::Separator();
		ImGui::Text("Action %zu", i);

		std::string currentId = actions[i].id;
		int currentIndex = -1;
		std::vector<const char*> labels;
		labels.reserve(actionDefs.size());
		for (size_t idx = 0; idx < actionDefs.size(); ++idx)
		{
			labels.push_back(actionDefs[idx].id.c_str());
			if (actionDefs[idx].id == currentId)
			{
				currentIndex = static_cast<int>(idx);
			}
		}

		if (!labels.empty())
		{
			if (ImGui::Combo("Action", &currentIndex, labels.data(), static_cast<int>(labels.size())))
			{
				actions[i].id = actionDefs[currentIndex].id;
				actions[i].params = nlohmann::json::object();
				updated = true;
			}
		}
		else
		{
			ImGui::TextDisabled("No Registered Actions");
		}

		updated |= DrawFSMActionParams(actions[i], categories);

		if (ImGui::Button("Remove Action"))
		{
			actions.erase(actions.begin() + static_cast<long>(i));
			updated = true;
			ImGui::PopID();
			break;
		}

		ImGui::PopID();
	}

	if (ImGui::Button("Add Action"))
	{
		FSMAction newAction;
		if (!actionDefs.empty())
		{
			newAction.id = actionDefs.front().id;
		}
		newAction.params = nlohmann::json::object();
		actions.push_back(newAction);
		updated = true;
	}

	ImGui::TreePop();
	return updated;
}

bool DrawFSMTransitions(std::vector<FSMTransition>& transitions, const std::vector<std::string>& stateNames, const std::vector<std::string>& categories)
{
	bool updated = false;
	if (!ImGui::TreeNode("Transitions"))
	{
		return false;
	}

	const auto& eventDefs = CollectFSMEventDefs(categories);
	std::vector<const char*> eventLabels;
	eventLabels.reserve(eventDefs.size());
	for (const auto& evt : eventDefs)
	{
		eventLabels.push_back(evt.name.c_str());
	}

	std::vector<const char*> stateLabels;
	stateLabels.reserve(stateNames.size());
	for (const auto& name : stateNames)
	{
		stateLabels.push_back(name.c_str());
	}

	for (size_t i = 0; i < transitions.size(); ++i)
	{
		ImGui::PushID(static_cast<int>(i));
		ImGui::Separator();
		ImGui::Text("Transition %zu", i);

		int eventIndex = -1;
		for (size_t idx = 0; idx < eventDefs.size(); ++idx)
		{
			if (eventDefs[idx].name == transitions[i].eventName)
			{
				eventIndex = static_cast<int>(idx);
				break;
			}
		}

		if (!eventLabels.empty())
		{
			if (ImGui::Combo("Event", &eventIndex, eventLabels.data(), static_cast<int>(eventLabels.size())))
			{
				transitions[i].eventName = eventDefs[eventIndex].name;
				updated = true;
			}
		}
		else
		{
			ImGui::TextDisabled("No Registerd Events");
		}

		int targetIndex = -1;
		for (size_t idx = 0; idx < stateNames.size(); ++idx)
		{
			if (stateNames[idx] == transitions[i].targetState)
			{
				targetIndex = static_cast<int>(idx);
				break;
			}
		}

		if (!stateLabels.empty())
		{
			if (ImGui::Combo("Target", &targetIndex, stateLabels.data(), static_cast<int>(stateLabels.size())))
			{
				transitions[i].targetState = stateNames[targetIndex];
				updated = true;
			}
		}
		else
		{
			ImGui::TextDisabled("No States Available");
		}

		if (ImGui::DragInt("Priority", &transitions[i].priority))
		{
			updated = true;
		}

if (ImGui::Button("Remove Transition"))
{
	transitions.erase(transitions.begin() + static_cast<long>(i));
	updated = true;
	ImGui::PopID();
	break;
}

ImGui::PopID();
	}

	if (ImGui::Button("Add Transition"))
	{
		FSMTransition newTransition;
		if (!eventDefs.empty())
		{
			newTransition.eventName = eventDefs.front().name;
		}
		if (!stateNames.empty())
		{
			newTransition.targetState = stateNames.front();
		}
		transitions.push_back(newTransition);
		updated = true;
	}

	ImGui::TreePop();
	return updated;
}


bool DrawFSMState(FSMState& state, const std::vector<std::string>& stateNames, const std::vector<std::string>& actionCategories, const std::vector<std::string>& eventCategories)
{
	bool updated = false;
	ImGui::Separator();
	ImGui::Text("State");

	std::array<char, 256> buffer{};
	CopyStringToBuffer(state.name, buffer);

	if (ImGui::InputText("Name", buffer.data(), buffer.size()))
	{
		state.name = buffer.data();
		updated = true;
	}

	updated |= DrawFSMActionList("OnEnter", state.onEnter, actionCategories);
	updated |= DrawFSMActionList("OnExit", state.onExit, actionCategories);
	updated |= DrawFSMTransitions(state.transitions, stateNames, eventCategories);

	return updated;
}

bool DrawFSMGraphEditor(FSMGraph& graph, const std::string& category)
{
	bool updated = false;
	std::vector<std::string> stateNames;
	stateNames.reserve(graph.states.size());

	std::vector<std::string> actionCategories;
	if (!category.empty())
	{
		actionCategories.push_back(category);
	}
	actionCategories.push_back("FSM");

	std::vector<std::string> eventCategories;
	if (!category.empty())
	{
		eventCategories.push_back(category);
	}
	eventCategories.push_back("Common");


	for (const auto& state : graph.states)
	{
		stateNames.push_back(state.name);
	}

	int initialIndex = -1;
	for (size_t idx = 0; idx < stateNames.size(); ++idx)
	{
		if (stateNames[idx] == graph.initialState)
		{
			initialIndex = static_cast<int>(idx);
			break;
		}
	}

	std::vector<const char*> stateLabels;
	stateLabels.reserve(stateNames.size());

	for (const auto& name : stateNames)
	{
		stateLabels.push_back(name.c_str());
	}

	if (!stateLabels.empty())
	{
		if (ImGui::Combo("Initial State", &initialIndex, stateLabels.data(), static_cast<int>(stateLabels.size())))
		{
			graph.initialState = stateNames[initialIndex];
			updated = true;
		}
	}
	else
	{
		ImGui::TextDisabled("No States Defined");
	}

	for (size_t i = 0; i < graph.states.size(); ++i)
	{
		ImGui::PushID(static_cast<int>(i));
		if (ImGui::TreeNode("State Editor", "State %zu", i))
		{
			updated |= DrawFSMState(graph.states[i], stateNames, actionCategories, eventCategories); 

			if (ImGui::Button("Remove State"))
			{
				graph.states.erase(graph.states.begin() + static_cast<long>(i));
				updated = true;
				ImGui::TreePop();
				ImGui::PopID();
				break;
			}
			ImGui::TreePop();
		}
		ImGui::PopID();
	}

	if (ImGui::Button("Add State"))
	{
		FSMState newState;
		newState.name = "NewState";
		graph.states.push_back(newState);
		updated = true;
	}

	return updated;
}

PropertyEditResult DrawComponentPropertyEditor(Component* component, const Property& property, AssetLoader& assetLoader)
{	// 각 Property별 배치 Layout은 정해줘야 함
	using PlaybackStateType = std::decay_t<decltype(std::declval<AnimationComponent>().GetPlayback())>;
	using BoneMaskSourceType = std::decay_t<decltype(std::declval<AnimationComponent>().GetBoneMaskSource())>;
	using RetargetOffsetsType = std::decay_t<decltype(std::declval<AnimationComponent>().GetRetargetOffsets())>;
	const std::type_info& typeInfo = property.GetTypeInfo();
	PropertyEditResult result;

	struct RotationUIState
	{
		XMFLOAT4 lastQuaternion{ 0.0f, 0.0f, 0.0f, 1.0f };
		XMFLOAT3 eulerDegrees{ 0.0f, 0.0f, 0.0f };
		bool initialized = false;
	};

	static std::unordered_map<const void*, RotationUIState> rotationUiState;

	auto resolveTextureDisplay = [&assetLoader](const TextureHandle& handle) -> std::string
		{
			const std::string* key = assetLoader.GetTextures().GetKey(handle);
			const std::string* displayName = assetLoader.GetTextures().GetDisplayName(handle);
			if (displayName && !displayName->empty())
			{
				return *displayName;
			}
			if (key && !key->empty())
			{
				return *key;
			}
			return "<None>";
		};

	auto drawTextureHandle = [&](const char* label, TextureHandle& value) -> bool
		{
			bool updated = false;
			const std::string display = resolveTextureDisplay(value);
			const std::string buttonLabel = display + "##" + label;

			ImGui::TextUnformatted(label);
			ImGui::SameLine();
			ImGui::Button(buttonLabel.c_str());
			result.activated = result.activated || ImGui::IsItemActivated();
			result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_TEXTURE"))
				{
					const TextureHandle dropped = *static_cast<const TextureHandle*>(payload->Data);
					value = dropped;
					updated = true;
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::SameLine();
			const std::string clearLabel = std::string("Clear##") + label;
			if (ImGui::Button(clearLabel.c_str()))
			{
				value = TextureHandle::Invalid();
				updated = true;
			}
			result.activated = result.activated || ImGui::IsItemActivated();
			result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
			return updated;
		};

	auto drawAnchor = [&](const char* label, UIAnchor& value) -> bool
		{
			float data[2] = { value.x, value.y };
			if (ImGui::DragFloat2(label, data, DRAG_SPEED, 0.0f, 1.0f))
			{
				value.x = data[0];
				value.y = data[1];
				return true;
			}
			return false;
		};

	auto drawRect = [&](const char* label, UIRect& value) -> bool
		{
			float data[4] = { value.x, value.y, value.width, value.height };
			if (ImGui::DragFloat4(label, data, DRAG_SPEED))
			{
				value.x = data[0];
				value.y = data[1];
				value.width = data[2];
				value.height = data[3];
				return true;
			}
			return false;
		};

	auto drawOffset = [&](const char* label, UIAnchor& value) -> bool
		{
			float data[2] = { value.x, value.y };
			if (ImGui::DragFloat2(label, data, DRAG_SPEED))
			{
				value.x = data[0];
				value.y = data[1];
				return true;
			}
			return false;
		};


	if (typeInfo == typeid(int))
	{
		int value = 0;
		property.GetValue(component, &value);
		if (ImGui::DragInt(property.GetName().c_str(), &value))
		{
			property.SetValue(component, &value);
			result.updated = true;
		}
		result.activated   = result.activated   || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		return result;
	}

	if (typeInfo == typeid(float))
	{
		float value = 0.0f;
		property.GetValue(component, &value);
		if (ImGui::DragFloat(property.GetName().c_str(), &value, DRAG_SPEED))
		{
			property.SetValue(component, &value);
			result.updated = true;
		}
		result.activated   = result.activated	|| ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		return result;
	}

	if (typeInfo == typeid(bool))
	{
		bool value = false;
		property.GetValue(component, &value);
		if (ImGui::Checkbox(property.GetName().c_str(), &value))
		{
			property.SetValue(component, &value);
			result.updated = true;
		}
		result.activated   = result.activated	|| ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		return result;
	}

	if (typeInfo == typeid(std::string))
	{
		std::string value;
		property.GetValue(component, &value);
		std::array<char, 256> buffer{};
		CopyStringToBuffer(value, buffer);
		if (ImGui::InputText(property.GetName().c_str(), buffer.data(), buffer.size()))
		{
			std::string updatedValue(buffer.data());
			property.SetValue(component, &updatedValue);
			result.updated = true;
		}
		result.activated   = result.activated	|| ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		return result;
	}

	if (typeInfo == typeid(XMFLOAT2))
	{
		XMFLOAT2 value{};
		property.GetValue(component, &value);
		float data[2] = { value.x, value.y };
		if (ImGui::DragFloat2(property.GetName().c_str(), data, DRAG_SPEED))
		{
			value.x = data[0];
			value.y = data[1];
			property.SetValue(component, &value);
			result.updated = true;
		}
		result.activated = result.activated || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		return result;
	}

	if (typeInfo == typeid(XMFLOAT3))
	{ 
		if (auto* transform = dynamic_cast<LightComponent*>(component)) {
			if (property.GetName() == "Color") {
				XMFLOAT3 value{};
				property.GetValue(component, &value);
				float data[3] = { value.x, value.y, value.z };
				if (ImGui::ColorEdit3(property.GetName().c_str(), data, DRAG_SPEED))
				{
					value.x = data[0];
					value.y = data[1];
					value.z = data[2];
					property.SetValue(component, &value);
					result.updated = true;
				}
				result.activated = result.activated || ImGui::IsItemActivated();
				result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
				return result;
			}

		}

		XMFLOAT3 value{};
		property.GetValue(component, &value);
		float data[3] = { value.x, value.y, value.z };
		if (ImGui::DragFloat3(property.GetName().c_str(), data, DRAG_SPEED))
		{
			value.x = data[0];
			value.y = data[1];
			value.z = data[2];
			property.SetValue(component, &value);
			result.updated = true;
		}
		result.activated   = result.activated	|| ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		return result;
	}

	if (typeInfo == typeid(XMFLOAT4))
	{
		if (auto* transform = dynamic_cast<TransformComponent*>(component))
		{

			// Rotation에 대한 GUI만 오일러로 변환
			if (property.GetName() == "Rotation")
			{
				XMFLOAT4 value{};
				property.GetValue(component, &value);
				auto& state = rotationUiState[component];
				const float diff = std::abs(state.lastQuaternion.x - value.x)
					+ std::abs(state.lastQuaternion.y - value.y)
					+ std::abs(state.lastQuaternion.z - value.z)
					+ std::abs(state.lastQuaternion.w - value.w);

				if (!state.initialized || diff > 1e-4f)
				{
					const XMFLOAT3 eulerRadians = QuaternionToEulerRadians(value);
					const XMFLOAT3 eulerDegrees = {
						XMConvertToDegrees(eulerRadians.x),
						XMConvertToDegrees(eulerRadians.y),
						XMConvertToDegrees(eulerRadians.z)
					};
					state.eulerDegrees = {
						NormalizeDegrees(eulerDegrees.x),
						NormalizeDegrees(eulerDegrees.y),
						NormalizeDegrees(eulerDegrees.z)
					};
					state.lastQuaternion = value;
					state.initialized = true;
				}

				float data[3] = {
					state.eulerDegrees.x,
					state.eulerDegrees.y,
					state.eulerDegrees.z
				};

				if (ImGui::DragFloat3(property.GetName().c_str(), data, 0.1f))
				{
					state.eulerDegrees = {
						NormalizeDegrees(data[0]),
						NormalizeDegrees(data[1]),
						NormalizeDegrees(data[2])
					};
					const XMFLOAT3 updatedRadians = {
						XMConvertToRadians(state.eulerDegrees.x),
						XMConvertToRadians(state.eulerDegrees.y),
						XMConvertToRadians(state.eulerDegrees.z)
					};
					const XMFLOAT4 updatedRotation = EulerRadiansToQuaternion(updatedRadians);
					property.SetValue(component, &updatedRotation);
					state.lastQuaternion = updatedRotation;
					result.updated = true;
				}
				result.activated   = result.activated	|| ImGui::IsItemActivated();
				result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
				return result;
			}
		}

		XMFLOAT4 value{};
		property.GetValue(component, &value);
		float data[4] = { value.x, value.y, value.z, value.w };
		if (ImGui::DragFloat4(property.GetName().c_str(), data, DRAG_SPEED))
		{
			value.x = data[0];
			value.y = data[1];
			value.z = data[2];
			value.w = data[3];
			property.SetValue(component, &value);
			result.updated = true;
		}
		result.activated   = result.activated   || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		return result;
	}

	if (typeInfo == typeid(UIAnchor))
	{
		UIAnchor value{};
		property.GetValue(component, &value);
		if (drawAnchor(property.GetName().c_str(), value))
		{
			property.SetValue(component, &value);
			result.updated = true;
		}
		result.activated = result.activated || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		return result;
	}

	if (typeInfo == typeid(UIRect))
	{
		UIRect value{};
		property.GetValue(component, &value);
		if (drawRect(property.GetName().c_str(), value))
		{
			property.SetValue(component, &value);
			result.updated = true;
		}
		result.activated = result.activated || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		return result;
	}


	// 카메라
	if (typeInfo == typeid(Viewport))
	{
		Viewport value{};
		property.GetValue(component, &value);
		float data[2] = { value.Width, value.Height };
		if (ImGui::DragFloat2(property.GetName().c_str(), data, DRAG_SPEED))
		{
			value.Width = data[0];
			value.Height = data[1];
			property.SetValue(component, &value);
			result.updated = true;
		}
		result.activated   = result.activated	|| ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		return result;
	}

	if (typeInfo == typeid(ProjectionMode))
	{
		ProjectionMode value{};
		property.GetValue(component, &value);
		static constexpr const char* kModes[] = { "Perspective", "Orthographic", "Ortho OffCenter" };
		int current = static_cast<int>(value);
		if (ImGui::Combo(property.GetName().c_str(), &current, kModes, IM_ARRAYSIZE(kModes)))
		{
			const int clamped = std::clamp(current, 0, static_cast<int>(IM_ARRAYSIZE(kModes) - 1));
			const ProjectionMode updated = static_cast<ProjectionMode>(clamped);
			property.SetValue(component, &updated);
			result.updated = true;
		}
		result.activated = result.activated || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		return result;
	}

	if (typeInfo == typeid(PerspectiveParams))
	{
		PerspectiveParams value{};
		property.GetValue(component, &value);
		bool updated = false;
		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::Indent();
		ImGui::PushID(property.GetName().c_str());
		updated |= ImGui::InputFloat("Fov", &value.Fov);
		result.activated   = result.activated   || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		updated |= ImGui::InputFloat("Aspect", &value.Aspect);
		result.activated   = result.activated   || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		ImGui::PopID();
		ImGui::Unindent();
		if (updated)
		{
			property.SetValue(component, &value);
			result.updated = true;
		}

		return result;
	}

	if (typeInfo == typeid(OrthoParams))
	{
		OrthoParams value{};
		property.GetValue(component, &value);
		bool updated = false;
		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::Indent();
		ImGui::PushID(property.GetName().c_str());
		updated |= ImGui::InputFloat("Width", &value.Width);
		result.activated   = result.activated   || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		updated |= ImGui::InputFloat("Height", &value.Height);
		result.activated   = result.activated   || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

		ImGui::PopID();
		ImGui::Unindent();
		if (updated)
		{
			property.SetValue(component, &value);
			result.updated = true;
		}

		return result;
	}

	if (typeInfo == typeid(OrthoOffCenterParams))
	{
		OrthoOffCenterParams value{};
		property.GetValue(component, &value);
		bool updated = false;
		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::Indent();
		ImGui::PushID(property.GetName().c_str());
		updated |= ImGui::InputFloat("Left", &value.Left);
		result.activated   = result.activated   || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		updated |= ImGui::InputFloat("Right", &value.Right);
		result.activated   = result.activated   || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		updated |= ImGui::InputFloat("Bottom", &value.Bottom);
		result.activated   = result.activated   || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		updated |= ImGui::InputFloat("Top", &value.Top);
		result.activated   = result.activated   || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		ImGui::PopID();
		ImGui::Unindent();
		if (updated)
		{
			property.SetValue(component, &value);
			result.updated = true;
		}

		return result;
	}


	// Render Layer
	if (typeInfo == typeid(UINT8))
	{
		UINT8 value = 0;
		property.GetValue(component, &value);
		if (property.GetName() == "RenderLayer")
		{
			static constexpr const char* kLayers[] = { "None", "Opaque", "Transparent", "Wall", "Refraction",  "Emissive", "UI" };
			int current = static_cast<int>(value);
			if (ImGui::Combo(property.GetName().c_str(), &current, kLayers, IM_ARRAYSIZE(kLayers)))
			{
				const UINT8 updated = static_cast<UINT8>(std::clamp(current, 0, static_cast<int>(IM_ARRAYSIZE(kLayers) - 1)));
				property.SetValue(component, &updated);
				result.updated = true;
			}
			result.activated   = result.activated   || ImGui::IsItemActivated();
			result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
			return result;
		}

		if (ImGui::DragScalar(property.GetName().c_str(), ImGuiDataType_U8, &value))
		{
			property.SetValue(component, &value);
			result.updated = true;
		}
		result.activated   = result.activated   || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		return result;
	}


	// AssetPath + AssetIndex
	if (typeInfo == typeid(AssetRef))
	{
		AssetRef value{};
		property.GetValue(component, &value);

		std::string label = property.GetName();
		label += ": ";
		if (!value.assetPath.empty())
		{
			label += value.assetPath + " [" + std::to_string(value.assetIndex) + "]";
		}
		else
		{
			label += "<None>";
		}
		ImGui::TextUnformatted(label.c_str());
		return result;
	}

	if (typeInfo == typeid(std::vector<std::string>))
	{
		ImGui::PushID(property.GetName().c_str());

		std::vector<std::string> value;
		property.GetValue(component, &value);
		bool updated = false;

		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::Indent();
		for (size_t i = 0; i < value.size(); ++i)
		{
			ImGui::PushID(static_cast<int>(i));
			std::array<char, 256> buffer{};
			CopyStringToBuffer(value[i], buffer);
			if (ImGui::InputText("Item", buffer.data(), buffer.size()))
			{
				value[i] = buffer.data();
				updated = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("Remove"))
			{
				value.erase(value.begin() + static_cast<long>(i));
				updated = true;
				ImGui::PopID();
				break;
			}
			ImGui::PopID();
		}
		if (ImGui::Button("Add Item"))
		{
			value.emplace_back();
			updated = true;
		}
		ImGui::Unindent();

		if (updated)
		{
			property.SetValue(component, &value);
			result.updated = true;
		}

		ImGui::PopID();
		return result;
	}

	if (typeInfo == typeid(std::vector<UIAnchor>))
	{
		std::vector<UIAnchor> value;
		property.GetValue(component, &value);
		bool updated = false;

		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::Indent();
		for (size_t i = 0; i < value.size(); ++i)
		{
			ImGui::PushID(static_cast<int>(i));
			const std::string label = "Offset " + std::to_string(i);
			updated |= drawOffset(label.c_str(), value[i]);
			ImGui::SameLine();
			if (ImGui::Button("Remove"))
			{
				value.erase(value.begin() + static_cast<long>(i));
				updated = true;
				ImGui::PopID();
				break;
			}
			ImGui::PopID();
		}
		if (ImGui::Button("Add Offset"))
		{
			value.push_back(UIAnchor{});
			updated = true;
		}
		ImGui::Unindent();

		if (updated)
		{
			property.SetValue(component, &value);
			result.updated = true;
		}
		return result;
	}

	if (typeInfo == typeid(std::array<TextureHandle, 10>))
	{
		std::array<TextureHandle, 10> value{};
		property.GetValue(component, &value);
		bool updated = false;
		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::Indent();
		for (size_t i = 0; i < value.size(); ++i)
		{
			ImGui::PushID(static_cast<int>(i));
			const std::string label = "Digit " + std::to_string(i);
			updated |= drawTextureHandle(label.c_str(), value[i]);
			ImGui::PopID();
		}
		ImGui::Unindent();
		if (updated)
		{
			property.SetValue(component, &value);
			result.updated = true;
		}
		return result;
	}

	if (typeInfo == typeid(std::array<float, 10>))
	{
		std::array<float, 10> value{};
		property.GetValue(component, &value);
		bool updated = false;
		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::Indent();
		for (size_t i = 0; i < value.size(); ++i)
		{
			ImGui::PushID(static_cast<int>(i));
			float item = value[i];
			const std::string label = "Digit " + std::to_string(i);
			if (ImGui::DragFloat(label.c_str(), &item, DRAG_SPEED))
			{
				value[i] = item;
				updated = true;
			}
			ImGui::PopID();
		}
		ImGui::Unindent();
		if (updated)
		{
			property.SetValue(component, &value);
			result.updated = true;
		}
		return result;
	}

	if (typeInfo == typeid(std::vector<UIDiceLayout>))
	{
		std::vector<UIDiceLayout> value;
		property.GetValue(component, &value);
		bool updated = false;

		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::Indent();
		for (size_t i = 0; i < value.size(); ++i)
		{
			ImGui::PushID(static_cast<int>(i));
			const std::string header = value[i].type.empty() ? "Layout" : value[i].type;
			if (ImGui::TreeNode("Layout", "%s %zu", header.c_str(), i))
			{
				std::array<char, 256> buffer{};
				CopyStringToBuffer(value[i].type, buffer);
				if (ImGui::InputText("Type", buffer.data(), buffer.size()))
				{
					value[i].type = buffer.data();
					updated = true;
				}
				updated |= drawTextureHandle("DiceTexture", value[i].diceTexture);

				if (ImGui::TreeNode("TensSlot"))
				{
					updated |= drawAnchor("Anchor", value[i].tens.anchor);
					updated |= drawAnchor("Pivot", value[i].tens.pivot);
					if (ImGui::Checkbox("UseParentOffset", &value[i].tens.useParentOffset))
					{
						updated = true;
					}

					updated |= drawRect("Bounds", value[i].tens.bounds);
					if (ImGui::TreeNode("DigitOffsets"))
					{
						for (size_t digit = 0; digit < value[i].tens.digitOffsets.size(); ++digit)
						{
							ImGui::PushID(static_cast<int>(digit));
							const std::string label = "Digit " + std::to_string(digit);
							updated |= drawOffset(label.c_str(), value[i].tens.digitOffsets[digit]);
							ImGui::PopID();
						}
						ImGui::TreePop();
					}
					ImGui::TreePop();
				}
				if (ImGui::TreeNode("OnesSlot"))
				{
					updated |= drawAnchor("Anchor", value[i].ones.anchor);
					updated |= drawAnchor("Pivot", value[i].ones.pivot);
					if (ImGui::Checkbox("UseParentOffset", &value[i].ones.useParentOffset))
					{
						updated = true;
					}

					updated |= drawRect("Bounds", value[i].ones.bounds);
					if (ImGui::TreeNode("DigitOffsets"))
					{
						for (size_t digit = 0; digit < value[i].ones.digitOffsets.size(); ++digit)
						{
							ImGui::PushID(static_cast<int>(digit));
							const std::string label = "Digit " + std::to_string(digit);
							updated |= drawOffset(label.c_str(), value[i].ones.digitOffsets[digit]);
							ImGui::PopID();
						}
						ImGui::TreePop();
					}
					ImGui::TreePop();
				}

				if (ImGui::Button("Remove Layout"))
				{
					value.erase(value.begin() + static_cast<long>(i));
					updated = true;
					ImGui::TreePop();
					ImGui::PopID();
					break;
				}
				ImGui::TreePop();
			}
			ImGui::PopID();
		}

		if (ImGui::Button("Add Layout"))
		{
			value.emplace_back();
			updated = true;
		}
		ImGui::Unindent();

		if (updated)
		{
			property.SetValue(component, &value);
			result.updated = true;
		}
		return result;
	}

	if (typeInfo == typeid(std::vector<UIDicePanelSlot>))
	{
		std::vector<UIDicePanelSlot> value;
		property.GetValue(component, &value);
		bool updated = false;

		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::Indent();
		for (size_t i = 0; i < value.size(); ++i)
		{
			ImGui::PushID(static_cast<int>(i));
			const std::string header = value[i].objectName.empty() ? "Slot" : value[i].objectName;
			if (ImGui::TreeNode("Slot", "%s %zu", header.c_str(), i))
			{
				std::array<char, 256> objectBuffer{};
				CopyStringToBuffer(value[i].objectName, objectBuffer);
				if (ImGui::InputText("ObjectName", objectBuffer.data(), objectBuffer.size()))
				{
					value[i].objectName = objectBuffer.data();
					updated = true;
				}

				std::array<char, 256> typeBuffer{};
				CopyStringToBuffer(value[i].diceType, typeBuffer);
				if (ImGui::InputText("DiceType", typeBuffer.data(), typeBuffer.size()))
				{
					value[i].diceType = typeBuffer.data();
					updated = true;
				}

				std::array<char, 256> contextBuffer{};
				CopyStringToBuffer(value[i].diceContext, contextBuffer);
				if (ImGui::InputText("DiceContext", contextBuffer.data(), contextBuffer.size()))
				{
					value[i].diceContext = contextBuffer.data();
					updated = true;
				}

				if (ImGui::Checkbox("ApplyAnimation", &value[i].applyAnimation))
				{
					updated = true;
				}

				if (ImGui::Button("Remove Slot"))
				{
					value.erase(value.begin() + static_cast<long>(i));
					updated = true;
					ImGui::TreePop();
					ImGui::PopID();
					break;
				}
				ImGui::TreePop();
			}
			ImGui::PopID();
		}

		if (ImGui::Button("Add Slot"))
		{
			value.emplace_back();
			updated = true;
		}
		ImGui::Unindent();

		if (updated)
		{
			property.SetValue(component, &value);
			result.updated = true;
		}
		return result;
	}

	if (typeInfo == typeid(XMFLOAT4X4))
	{
		XMFLOAT4X4 value{};
		property.GetValue(component, &value);
		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::Text("  [%.3f %.3f %.3f %.3f]", value._11, value._12, value._13, value._14);
		ImGui::Text("  [%.3f %.3f %.3f %.3f]", value._21, value._22, value._23, value._24);
		ImGui::Text("  [%.3f %.3f %.3f %.3f]", value._31, value._32, value._33, value._34);
		ImGui::Text("  [%.3f %.3f %.3f %.3f]", value._41, value._42, value._43, value._44);
		return result;
	}


	if (typeInfo == typeid(MeshHandle))
	{
		MeshHandle value{};
		property.GetValue(component, &value);

		const std::string* key = assetLoader.GetMeshes().GetKey(value);
		const std::string* displayName = assetLoader.GetMeshes().GetDisplayName(value);

		const char* name = (displayName && !displayName->empty()) ? displayName->c_str() : (key && !key->empty()) ? key->c_str() : "<None>";
		const std::string buttonLabel = std::string(name) + "##" + property.GetName();

		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::SameLine();
		ImGui::Button(buttonLabel.c_str());
		result.activated   = result.activated   || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

		if (ImGui::BeginDragDropTarget())
		{
			bool updated = false;
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_MESH"))
			{
				const MeshHandle dropped = *static_cast<const MeshHandle*>(payload->Data);
				property.SetValue(component, &dropped);

				if (auto* meshComponent = dynamic_cast<MeshComponent*>(component))
				{
					const std::string* droppedKey = assetLoader.GetMeshes().GetKey(dropped);
				}

				updated = true;
			}
			ImGui::EndDragDropTarget();
			if (updated)
			{
				result.updated = true;
			}
		}

		return result;
	}

	if (typeInfo == typeid(MaterialHandle))
	{
		MaterialHandle value{};
		property.GetValue(component, &value);

		const std::string* key = assetLoader.GetMaterials().GetKey(value);
		const std::string* displayName = assetLoader.GetMaterials().GetDisplayName(value);

		const char* name = (displayName && !displayName->empty()) ? displayName->c_str() : (key && !key->empty()) ? key->c_str() : "<None>";
		const std::string buttonLabel = std::string(name) + "##" + property.GetName();

		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::SameLine();
		ImGui::Button(buttonLabel.c_str());
		result.activated   = result.activated   || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

		if (ImGui::BeginDragDropTarget())
		{
			bool updated = false;
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_MATERIAL"))
			{
				const MaterialHandle dropped = *static_cast<const MaterialHandle*>(payload->Data);
				property.SetValue(component, &dropped);

				if (auto* materialComponent = dynamic_cast<MaterialComponent*>(component))
				{
					const std::string* droppedKey = assetLoader.GetMaterials().GetKey(dropped);
				}

				updated = true;
			}
			ImGui::EndDragDropTarget();
			if (updated)
			{
				result.updated = true;
			}
		}

		return result;
	}

	if (typeInfo == typeid(TextureHandle))
	{
		TextureHandle value{};
		property.GetValue(component, &value);

		const std::string* key = assetLoader.GetTextures().GetKey(value);
		const std::string* displayName = assetLoader.GetTextures().GetDisplayName(value);

		const char* name = (displayName && !displayName->empty()) ? displayName->c_str() : (key && !key->empty()) ? key->c_str() : "<None>";
		const std::string buttonLabel = std::string(name) + "##" + property.GetName();

		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::SameLine();
		ImGui::Button(buttonLabel.c_str());
		result.activated   = result.activated   || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

		if (ImGui::BeginDragDropTarget())
		{
			bool updated = false;
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_TEXTURE"))
			{
				const TextureHandle dropped = *static_cast<const TextureHandle*>(payload->Data);
				property.SetValue(component, &dropped);
				updated = true;
			}
			ImGui::EndDragDropTarget();
			if (updated)
			{
				result.updated = true;
			}
		}

		return result;
	}


	if (typeInfo == typeid(VertexShaderHandle))
	{
		VertexShaderHandle value{};
		property.GetValue(component, &value);

		const std::string* key = assetLoader.GetVertexShaders().GetKey(value);
		const std::string* displayName = assetLoader.GetVertexShaders().GetDisplayName(value);

		const char* name = (displayName && !displayName->empty()) ? displayName->c_str() : (key && !key->empty()) ? key->c_str() : "<None>";
		const std::string buttonLabel = std::string(name) + "##" + property.GetName();

		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::SameLine();
		ImGui::Button(buttonLabel.c_str());
		result.activated = result.activated || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

		if (ImGui::BeginDragDropTarget())
		{
			bool updated = false;
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_VERTEX_SHADER"))
			{
				const VertexShaderHandle dropped = *static_cast<const VertexShaderHandle*>(payload->Data);
				property.SetValue(component, &dropped);
				updated = true;
			}
			ImGui::EndDragDropTarget();
			if (updated)
			{
				result.updated = true;
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Clear##VertexShader"))
		{
			value = VertexShaderHandle::Invalid();
			property.SetValue(component, &value);
			result.updated = true;
		}
		result.activated = result.activated || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

		return result;
	}

	if (typeInfo == typeid(PixelShaderHandle))
	{
		PixelShaderHandle value{};
		property.GetValue(component, &value);

		const std::string* key = assetLoader.GetPixelShaders().GetKey(value);
		const std::string* displayName = assetLoader.GetPixelShaders().GetDisplayName(value);

		const char* name = (displayName && !displayName->empty()) ? displayName->c_str() : (key && !key->empty()) ? key->c_str() : "<None>";
		const std::string buttonLabel = std::string(name) + "##" + property.GetName();

		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::SameLine();
		ImGui::Button(buttonLabel.c_str());
		result.activated = result.activated || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

		if (ImGui::BeginDragDropTarget())
		{
			bool updated = false;
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_PIXEL_SHADER"))
			{
				const PixelShaderHandle dropped = *static_cast<const PixelShaderHandle*>(payload->Data);
				property.SetValue(component, &dropped);
				updated = true;
			}
			ImGui::EndDragDropTarget();
			if (updated)
			{
				result.updated = true;
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Clear##PixelShader"))
		{
			value = PixelShaderHandle::Invalid();
			property.SetValue(component, &value);
			result.updated = true;
		}
		result.activated = result.activated || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

		return result;
	}

	if (typeInfo == typeid(ShaderAssetHandle))
	{
		ShaderAssetHandle value{};
		property.GetValue(component, &value);

		const std::string* key = assetLoader.GetShaderAssets().GetKey(value);
		const std::string* displayName = assetLoader.GetShaderAssets().GetDisplayName(value);

		const char* name = (displayName && !displayName->empty()) ? displayName->c_str() : (key && !key->empty()) ? key->c_str() : "<None>";
		const std::string buttonLabel = std::string(name) + "##" + property.GetName();

		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::SameLine();
		ImGui::Button(buttonLabel.c_str());
		result.activated = result.activated || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

		if (ImGui::BeginDragDropTarget())
		{
			bool updated = false;
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_SHADER_ASSET"))
			{
				const ShaderAssetHandle dropped = *static_cast<const ShaderAssetHandle*>(payload->Data);
				property.SetValue(component, &dropped);
				updated = true;
			}
			ImGui::EndDragDropTarget();
			if (updated)
			{
				result.updated = true;
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Clear##ShaderAsset"))
		{
			value = ShaderAssetHandle::Invalid();
			property.SetValue(component, &value);
			result.updated = true;
		}
		result.activated = result.activated || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

		return result;
	}


	if (typeInfo == typeid(AnimationHandle))
	{
		AnimationHandle value{};
		property.GetValue(component, &value);

		const std::string* key = assetLoader.GetAnimations().GetKey(value);
		const std::string* displayName = assetLoader.GetAnimations().GetDisplayName(value);

		const char* name = (displayName && !displayName->empty()) ? displayName->c_str() : (key && !key->empty()) ? key->c_str() : "<None>";
		const std::string buttonLabel = std::string(name) + "##" + property.GetName();

		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::SameLine();
		ImGui::Button(buttonLabel.c_str());
		result.activated   = result.activated   || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

		if (ImGui::BeginDragDropTarget())
		{
			bool updated = false;
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_ANIMATION"))
			{
				const AnimationHandle dropped = *static_cast<const AnimationHandle*>(payload->Data);
				property.SetValue(component, &dropped);
				updated = true;
			}
			ImGui::EndDragDropTarget();
			if (updated)
			{
				result.updated = true;
			}
		}

		return result;
	}


	if (typeInfo == typeid(SkeletonHandle))
	{
		SkeletonHandle value{};
		property.GetValue(component, &value);

		const std::string* key = assetLoader.GetSkeletons().GetKey(value);
		const std::string* displayName = assetLoader.GetSkeletons().GetDisplayName(value);

		const char* name = (displayName && !displayName->empty()) ? displayName->c_str() : (key && !key->empty()) ? key->c_str() : "<None>";
		const std::string buttonLabel = std::string(name) + "##" + property.GetName();

		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::SameLine();
		ImGui::Button(buttonLabel.c_str());
		result.activated   = result.activated   || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

		if (ImGui::BeginDragDropTarget())
		{
			bool updated = false;
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_SKELETON"))
			{
				const SkeletonHandle dropped = *static_cast<const SkeletonHandle*>(payload->Data);
				property.SetValue(component, &dropped);
				updated = true;
			}
			ImGui::EndDragDropTarget();
			if (updated)
			{
				result.updated = true;
			}
		}

		return result;
	}



	if (typeInfo == typeid(RenderData::MaterialData))
	{
		RenderData::MaterialData value{};
		property.GetValue(component, &value);

		bool updated = false;
		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::Indent();
		ImGui::PushID(property.GetName().c_str());

		updated |= ImGui::ColorEdit4("Base Color", &value.baseColor.x);
		result.activated   = result.activated   || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

		updated |= ImGui::DragFloat("Metallic", &value.metallic, 0.01f, 0.0f, 1.0f);
		result.activated   = result.activated   || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

		updated |= ImGui::DragFloat("Roughness", &value.roughness, 0.01f, 0.0f, 1.0f);
		result.activated   = result.activated   || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

		updated |= ImGui::DragFloat("Saturation", &value.saturation, 0.01f, 0.0f, 10.0f);
		result.activated = result.activated || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

		updated |= ImGui::DragFloat("Lightness", &value.lightness, 0.01f, 0.0f, 10.0f);
		result.activated = result.activated || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

		// Texture slots
		static constexpr const char* kTextureLabels[] = { "Albedo", "Normal", "Metallic", "Roughness", "AO", "Emissive"};
		static_assert(std::size(kTextureLabels) == static_cast<size_t>(RenderData::MaterialTextureSlot::TEX_MAX));

		for (size_t i = 0; i < value.textures.size(); ++i)
		{
			ImGui::PushID(static_cast<int>(i));
			TextureHandle handle = value.textures[i];
			const std::string* key = assetLoader.GetTextures().GetKey(handle);
			const std::string* displayName = assetLoader.GetTextures().GetDisplayName(handle);

			const char* name = (displayName && !displayName->empty()) ? displayName->c_str() : (key && !key->empty()) ? key->c_str() : "<None>";
			const std::string buttonLabel = std::string(name) + "##MaterialTexture";

			ImGui::TextUnformatted(kTextureLabels[i]);
			ImGui::SameLine();
			ImGui::Button(buttonLabel.c_str());
			result.activated   = result.activated   || ImGui::IsItemActivated();
			result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_TEXTURE"))
				{
					const TextureHandle dropped = *static_cast<const TextureHandle*>(payload->Data);
					value.textures[i] = dropped;
					updated = true;
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::SameLine();
			if (ImGui::Button("Clear##Texture"))
			{
				value.textures[i] = TextureHandle::Invalid();
				updated = true;
			}
			result.activated   = result.activated   || ImGui::IsItemActivated();
			result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();


			ImGui::PopID();
		}

		bool stageShaderChanged = false;

		// Vertex shader handle
		{
			VertexShaderHandle shader = value.vertexShader;
			const std::string* key = assetLoader.GetVertexShaders().GetKey(shader);
			const std::string* displayName = assetLoader.GetVertexShaders().GetDisplayName(shader);
			const char* name = (displayName && !displayName->empty()) ? displayName->c_str() : (key && !key->empty()) ? key->c_str() : "<None>";
			const std::string buttonLabel = std::string(name) + "##MaterialVertexShader";

			ImGui::TextUnformatted("Vertex Shader");
			ImGui::SameLine();
			ImGui::Button(buttonLabel.c_str());

			result.activated   = result.activated   || ImGui::IsItemActivated();
			result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_VERTEX_SHADER"))
				{
					const VertexShaderHandle dropped = *static_cast<const VertexShaderHandle*>(payload->Data);
					value.vertexShader = dropped;
					stageShaderChanged = true;
					updated = true;
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::SameLine();
			if (ImGui::Button("Clear##VertexShader"))
			{
				value.vertexShader = VertexShaderHandle::Invalid();
				stageShaderChanged = true;
				updated = true;
			}
			result.activated   = result.activated   || ImGui::IsItemActivated();
			result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		}

		// Pixel shader handle
		{
			PixelShaderHandle shader = value.pixelShader;
			const std::string* key = assetLoader.GetPixelShaders().GetKey(shader);
			const std::string* displayName = assetLoader.GetPixelShaders().GetDisplayName(shader);
			const char* name = (displayName && !displayName->empty()) ? displayName->c_str() : (key && !key->empty()) ? key->c_str() : "<None>";
			const std::string buttonLabel = std::string(name) + "##MaterialPixelShader";

			ImGui::TextUnformatted("Pixel Shader");
			ImGui::SameLine();
			ImGui::Button(buttonLabel.c_str());

			result.activated   = result.activated   || ImGui::IsItemActivated();
			result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_PIXEL_SHADER"))
				{
					const PixelShaderHandle dropped = *static_cast<const PixelShaderHandle*>(payload->Data);
					value.pixelShader = dropped;
					stageShaderChanged = true;
					updated = true;
			
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::SameLine();
			if (ImGui::Button("Clear##PixelShader"))
			{
				value.pixelShader = PixelShaderHandle::Invalid();
				stageShaderChanged = true;
				updated = true;
			}
			result.activated   = result.activated   || ImGui::IsItemActivated();
			result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		}

		// Shader asset handle
		{
			ShaderAssetHandle shader = value.shaderAsset;
			const std::string* key = assetLoader.GetShaderAssets().GetKey(shader);
			const std::string* displayName = assetLoader.GetShaderAssets().GetDisplayName(shader);
			const char* name = (displayName && !displayName->empty()) ? displayName->c_str() : (key && !key->empty()) ? key->c_str() : "<None>";
			const std::string buttonLabel = std::string(name) + "##MaterialShaderAsset";

			ImGui::TextUnformatted("Shader Asset");
			ImGui::SameLine();
			ImGui::Button(buttonLabel.c_str());

			result.activated   = result.activated   || ImGui::IsItemActivated();
			result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_SHADER_ASSET"))
				{
					const ShaderAssetHandle dropped = *static_cast<const ShaderAssetHandle*>(payload->Data);
					value.shaderAsset = dropped;
					updated = true;
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::SameLine();
			if (ImGui::Button("Clear##ShaderAsset"))
			{
				value.shaderAsset = ShaderAssetHandle::Invalid();
				updated = true;
			}

			result.activated   = result.activated   || ImGui::IsItemActivated();
			result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		}

		ImGui::PopID();
		ImGui::Unindent();

		if (updated && !value.shaderAsset.IsValid() && value.vertexShader.IsValid() && value.pixelShader.IsValid())
		{
			value.shaderAsset = ShaderAssetHandle::Invalid();
		}
		if (updated)
		{
			property.SetValue(component, &value);
			result.updated = true;
		}
		return result;
	}


	if (typeInfo == typeid(std::vector<float>))
	{
		std::vector<float> value;
		property.GetValue(component, &value);
		ImGui::Text("%s: %zu entries", property.GetName().c_str(), value.size());
		return result;
	}

	if (typeInfo == typeid(std::vector<DirectX::XMFLOAT4X4>))
	{
		std::vector<DirectX::XMFLOAT4X4> value;
		property.GetValue(component, &value);
		ImGui::Text("%s: %zu matrices", property.GetName().c_str(), value.size());
		ImGui::PushID(property.GetName().c_str());

		static std::unordered_map<std::string, int> matrixIndices;
		int& index = matrixIndices[property.GetName()];
		if (index < 0)
		{
			index = 0;
		}

		if (!value.empty())
		{
			if (ImGui::InputInt("Index", &index))
			{
				index = std::clamp(index, 0, static_cast<int>(value.size() - 1));
			}

			if (ImGui::TreeNode("Matrix Values"))
			{
				const auto& m = value[static_cast<size_t>(index)];
				ImGui::Text("Row 0: %.3f %.3f %.3f %.3f", m._11, m._12, m._13, m._14);
				ImGui::Text("Row 1: %.3f %.3f %.3f %.3f", m._21, m._22, m._23, m._24);
				ImGui::Text("Row 2: %.3f %.3f %.3f %.3f", m._31, m._32, m._33, m._34);
				ImGui::Text("Row 3: %.3f %.3f %.3f %.3f", m._41, m._42, m._43, m._44);
				ImGui::TreePop();
			}
		}
		else
		{
			ImGui::TextDisabled("No matrices available.");
		}

		ImGui::PopID();
		return result;
	}

	if (typeInfo == typeid(std::vector<UIFSMEventCallback>))
	{
		std::vector<UIFSMEventCallback> value;
		property.GetValue(component, &value);

		bool updated = false;

		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::Indent();
		ImGui::PushID(property.GetName().c_str());

		const auto& eventDefs = FSMEventRegistry::Instance().GetEvents("UI");
		std::vector<std::string> uiEventNames;
		uiEventNames.reserve(eventDefs.size());
		for (const auto& def : eventDefs)
		{
			uiEventNames.push_back(def.name);
		}

		size_t index = 0;
		while (index < value.size())
		{
			ImGui::PushID(static_cast<int>(index));
			ImGui::Separator();

			const char* preview = value[index].eventName.empty() ? "<None>" : value[index].eventName.c_str();
			if (ImGui::BeginCombo("Event", preview))
			{
				for (const auto& name : uiEventNames)
				{
					const bool isSelected = (value[index].eventName == name);
					if (ImGui::Selectable(name.c_str(), isSelected))
					{
						value[index].eventName = name;
						updated = true;
					}
					if (isSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			result.activated = result.activated || ImGui::IsItemActivated();
			result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

			std::array<char, 256> callbackBuffer{};
			CopyStringToBuffer(value[index].callbackId, callbackBuffer);
			if (ImGui::InputText("Callback", callbackBuffer.data(), callbackBuffer.size()))
			{
				value[index].callbackId = callbackBuffer.data();
				updated = true;
			}
			result.activated = result.activated || ImGui::IsItemActivated();
			result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

			if (ImGui::Button("Remove"))
			{
				value.erase(value.begin() + static_cast<long long>(index));
				updated = true;
				ImGui::PopID();
				continue;
			}
			result.activated = result.activated || ImGui::IsItemActivated();
			result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

			ImGui::PopID();
			++index;
		}

		if (ImGui::Button("Add Callback"))
		{
			UIFSMEventCallback entry{};
			if (!uiEventNames.empty())
			{
				entry.eventName = uiEventNames.front();
			}
			value.push_back(std::move(entry));
			updated = true;
		}
		result.activated = result.activated || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

		ImGui::PopID();
		ImGui::Unindent();

		if (updated)
		{
			property.SetValue(component, &value);
			result.updated = true;
		}
		return result;
	}

	if (typeInfo == typeid(std::vector<UIFSMCallbackAction>))
	{
		std::vector<UIFSMCallbackAction> value;
		property.GetValue(component, &value);

		bool updated = false;

		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::Indent();
		ImGui::PushID(property.GetName().c_str());

		size_t index = 0;
		while (index < value.size())
		{
			ImGui::PushID(static_cast<int>(index));
			ImGui::Separator();

			std::array<char, 256> callbackBuffer{};
			CopyStringToBuffer(value[index].callbackId, callbackBuffer);
			if (ImGui::InputText("Callback", callbackBuffer.data(), callbackBuffer.size()))
			{
				value[index].callbackId = callbackBuffer.data();
				updated = true;
			}
			result.activated = result.activated || ImGui::IsItemActivated();
			result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

			ImGui::TextUnformatted("Actions");
			ImGui::Indent();
			auto& registry = FSMActionRegistry::Instance();
			
			std::vector<std::string> category;
			for (auto& action : registry.GetActions("UI"))
			{
				category.push_back(action.category);
			}
			updated |= DrawFSMActionList("##CallbackActions", value[index].actions, category);
			ImGui::Unindent();

			result.activated = result.activated || ImGui::IsItemActivated();
			result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

			if (ImGui::Button("Remove"))
			{
				value.erase(value.begin() + static_cast<long long>(index));
				updated = true;
				ImGui::PopID();
				continue;
			}
			result.activated = result.activated || ImGui::IsItemActivated();
			result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

			ImGui::PopID();
			++index;
		}

		if (ImGui::Button("Add Callback Action"))
		{
			value.emplace_back();
			updated = true;
		}
		result.activated = result.activated || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

		ImGui::PopID();
		ImGui::Unindent();

		if (updated)
		{
			property.SetValue(component, &value);
			result.updated = true;
		}
		return result;
	}

	if (typeInfo == typeid(FSMGraph))
	{
		FSMGraph graph;
		property.GetValue(component, &graph);
		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::Indent();
		ImGui::PushID(property.GetName().c_str());
		const std::string category = component ? ResolveFSMCategory(*component) : std::string{};
		const bool updated = DrawFSMGraphEditor(graph, category);

		result.activated   = result.activated   || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

		ImGui::PopID();
		ImGui::Unindent();
		if (updated)
		{
			property.SetValue(component, &graph);
			result.updated = true;
		}
		return result;
	}

	if (typeInfo == typeid(PlaybackStateType))
	{
		PlaybackStateType value{};
		property.GetValue(component, &value);
		ImGui::Text("%s", property.GetName().c_str());

		bool changed = false;

		changed |= ImGui::InputFloat("Time", &value.time, 0.01f, 0.1f, "%.3f");
		changed |= ImGui::InputFloat("Speed", &value.speed, 0.01f, 0.1f, "%.2f");

		changed |= ImGui::Checkbox("Looping", &value.looping);
		changed |= ImGui::Checkbox("Playing", &value.playing);
		changed |= ImGui::Checkbox("Reverse", &value.reverse);

		if (changed)
		{
			property.SetValue(component, &value);
			result.updated = true;
		}

		return result;
	}

	if (typeInfo == typeid(AnimationComponent::BlendState))
	{
		AnimationComponent::BlendState value{};
		property.GetValue(component, &value);

		bool updated = false;

		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::Indent();
		ImGui::PushID(property.GetName().c_str());

		// active
		updated |= ImGui::Checkbox("Active", &value.active);
		result.activated   = result.activated   || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

		// fromClip
		{
			const std::string* key = assetLoader.GetAnimations().GetKey(value.fromClip);
			const std::string display = key ? *key : std::string("<None>");
			const std::string buttonLabel = display + "##BlendFromClip";

			ImGui::TextUnformatted("From Clip");
			ImGui::SameLine();
			ImGui::Button(buttonLabel.c_str());

			result.activated   = result.activated   || ImGui::IsItemActivated();
			result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_ANIMATION"))
				{
					const AnimationHandle dropped = *static_cast<const AnimationHandle*>(payload->Data);
					value.fromClip = dropped;
					updated = true;
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::SameLine();
			if (ImGui::Button("Clear##BlendFromClip"))
			{
				value.fromClip = AnimationHandle::Invalid();
				updated = true;
			}

			result.activated   = result.activated   || ImGui::IsItemActivated();
			result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		}

		// toClip
		{
			const std::string* key = assetLoader.GetAnimations().GetKey(value.toClip);
			const std::string display = key ? *key : std::string("<None>");
			const std::string buttonLabel = display + "##BlendToClip";

			ImGui::TextUnformatted("To Clip");
			ImGui::SameLine();
			ImGui::Button(buttonLabel.c_str());
			
			result.activated   = result.activated   || ImGui::IsItemActivated();
			result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_ANIMATION"))
				{
					const AnimationHandle dropped = *static_cast<const AnimationHandle*>(payload->Data);
					value.toClip = dropped;
					updated = true;
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::SameLine();
			if (ImGui::Button("Clear##BlendToClip"))
			{
				value.toClip = AnimationHandle::Invalid();
				updated = true;
			}

			result.activated   = result.activated   || ImGui::IsItemActivated();
			result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		}

		// duration / elapsed / fromTime / toTime
		updated |= ImGui::InputFloat("Duration", &value.duration);
		result.activated   = result.activated   || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

		updated |= ImGui::InputFloat("Elapsed", &value.elapsed);
		result.activated   = result.activated   || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

		updated |= ImGui::InputFloat("From Time", &value.fromTime);
		result.activated   = result.activated   || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

		updated |= ImGui::InputFloat("To Time", &value.toTime);
		result.activated   = result.activated   || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

		// blendType (enum)
		{
			// enum -> int combo
			// BlendType 정의에 맞게 라벨/개수 수정 필요
			// 여기선 Linear, EaseIn, EaseOut, EaseInOut, Curve 가정
			static constexpr const char* kBlendTypeLabels[] =
			{
				"Linear",
				"EaseIn",
				"EaseOut",
				"EaseInOut",
				"Curve"
			};

			int current = static_cast<int>(value.blendType);
			if (ImGui::Combo("Blend Type", &current, kBlendTypeLabels, IM_ARRAYSIZE(kBlendTypeLabels)))
			{
				value.blendType = static_cast<AnimationComponent::BlendType>(current);
				updated = true;
			}
			result.activated   = result.activated   || ImGui::IsItemActivated();
			result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		}

		// curveFn: function pointer -> edit 불가 (표시만)
		{
			const bool hasCurve = (value.curveFn != nullptr);
			ImGui::Text("Curve Fn: %s", hasCurve ? "Set" : "None");
			// 필요하면 주소 표시 (디버그용)
			// ImGui::Text("Curve Fn Ptr: 0x%p", reinterpret_cast<void*>(value.curveFn));
		}

		ImGui::PopID();
		ImGui::Unindent();

		if (updated)
		{
			property.SetValue(component, &value);
			result.updated = true;
		}
		return result;
	}

	if (typeInfo == typeid(AnimationComponent::BlendConfig))
	{
		AnimationComponent::BlendConfig value{};
		property.GetValue(component, &value);

		bool updated = false;

		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::Indent();
		ImGui::PushID(property.GetName().c_str());

		{
			const std::string* key = assetLoader.GetAnimations().GetKey(value.fromClip);
			const std::string display = key ? *key : std::string("<None>");
			const std::string buttonLabel = display + "##BlendConfigFromClip";

			ImGui::TextUnformatted("From Clip");
			ImGui::SameLine();
			ImGui::Button(buttonLabel.c_str());

			result.activated   = result.activated   || ImGui::IsItemActivated();
			result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_ANIMATION"))
				{
					const AnimationHandle dropped = *static_cast<const AnimationHandle*>(payload->Data);
					value.fromClip = dropped;
					updated = true;
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::SameLine();
			if (ImGui::Button("Clear##BlendConfigFromClip"))
			{
				value.fromClip = AnimationHandle::Invalid();
				updated = true;
			}

			result.activated   = result.activated   || ImGui::IsItemActivated();
			result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		}

		{
			const std::string* key = assetLoader.GetAnimations().GetKey(value.toClip);
			const std::string display = key ? *key : std::string("<None>");
			const std::string buttonLabel = display + "##BlendConfigToClip";

			ImGui::TextUnformatted("To Clip");
			ImGui::SameLine();
			ImGui::Button(buttonLabel.c_str());

			result.activated   = result.activated   || ImGui::IsItemActivated();
			result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_ANIMATION"))
				{
					const AnimationHandle dropped = *static_cast<const AnimationHandle*>(payload->Data);
					value.toClip = dropped;
					updated = true;
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::SameLine();
			if (ImGui::Button("Clear##BlendConfigToClip"))
			{
				value.toClip = AnimationHandle::Invalid();
				updated = true;
			}

			result.activated   = result.activated   || ImGui::IsItemActivated();
			result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		}

		updated |= ImGui::InputFloat("Blend Time", &value.blendTime);

		{
			static constexpr const char* kBlendTypeLabels[] =
			{
				"Linear",
				"Curve"
			};

			int current = static_cast<int>(value.blendType);
			if (ImGui::Combo("Blend Type", &current, kBlendTypeLabels, IM_ARRAYSIZE(kBlendTypeLabels)))
			{
				value.blendType = static_cast<AnimationComponent::BlendType>(current);
				updated = true;
			}
			result.activated   = result.activated   || ImGui::IsItemActivated();
			result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		}

		{
			static constexpr const char* kCurveLabels[] =
			{
				"Linear",
				"EaseInSine",
				"EaseOutSine",
				"EaseInOutSine",
				"EaseInQuad",
				"EaseOutQuad",
				"EaseInOutQuad",
				"EaseInCubic",
				"EaseOutCubic",
				"EaseInOutCubic",
				"EaseInQuart",
				"EaseOutQuart",
				"EaseInOutQuart",
				"EaseInQuint",
				"EaseOutQuint",
				"EaseInOutQuint",
				"EaseInExpo",
				"EaseOutExpo",
				"EaseInOutExpo",
				"EaseInCirc",
				"EaseOutCirc",
				"EaseInOutCirc",
				"EaseInBack",
				"EaseOutBack",
				"EaseInOutBack",
				"EaseInElastic",
				"EaseOutElastic",
				"EaseInElastic",
				"EaseInBounce" ,
				"EaseOutBounce",
				"EaseInOutBounce"
			};

			int curveIndex = 0;
			for (int i = 0; i < IM_ARRAYSIZE(kCurveLabels); ++i)
			{
				if (value.curveName == kCurveLabels[i])
				{
					curveIndex = i;
					break;
				}
			}

			if (ImGui::Combo("Curve", &curveIndex, kCurveLabels, IM_ARRAYSIZE(kCurveLabels)))
			{
				value.curveName = kCurveLabels[curveIndex];
				updated = true;
			}
			result.activated   = result.activated   || ImGui::IsItemActivated();
			result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		}

		ImGui::PopID();
		ImGui::Unindent();

		if (updated)
		{
			property.SetValue(component, &value);
			result.updated = true;
		}
		return result;
	}

	if (typeInfo == typeid(BoneMaskSourceType))
	{
		BoneMaskSourceType value{};
		property.GetValue(component, &value);
		const char* label = "None";
		switch (value)
		{
		case AnimationComponent::BoneMaskSource::UpperBody:
			label = "UpperBody";
			break;
		case AnimationComponent::BoneMaskSource::LowerBody:
			label = "LowerBody";
			break;
		default:
			break;
		}
		ImGui::Text("%s: %s", property.GetName().c_str(), label);
		return result;
	}

	if (typeInfo == typeid(RetargetOffsetsType))
	{
		RetargetOffsetsType value;
		property.GetValue(component, &value);
		ImGui::Text("%s: %zu offsets", property.GetName().c_str(), value.size());
		return result;
	}

	if (typeInfo == typeid(UISize))
	{
		UISize value{};
		property.GetValue(component, &value);
		float data[2] = { value.width, value.height };
		if (ImGui::DragFloat2(property.GetName().c_str(), data, DRAG_SPEED))
		{
			value.width = data[0];
			value.height = data[1];
			property.SetValue(component, &value);
			result.updated = true;
		}
		result.activated = result.activated || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		return result;
	}

	if (typeInfo == typeid(UIPadding))
	{
		UIPadding value{};
		property.GetValue(component, &value);
		float data[4] = { value.left, value.top, value.right, value.bottom };
		if (ImGui::DragFloat4(property.GetName().c_str(), data, DRAG_SPEED))
		{
			value.left = data[0];
			value.top = data[1];
			value.right = data[2];
			value.bottom = data[3];
			property.SetValue(component, &value);
			result.updated = true;
		}
		result.activated = result.activated || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		return result;
	}

	if (typeInfo == typeid(std::vector<HorizontalBoxSlot>))
	{
		std::vector<HorizontalBoxSlot> slots;
		property.GetValue(component, &slots);
		bool updated = false;
		bool activated = false;
		bool deactivated = false;

		auto alignmentLabel = [](UIHorizontalAlignment alignment)
			{
				switch (alignment)
				{
				case UIHorizontalAlignment::Left:
					return "Left";
				case UIHorizontalAlignment::Center:
					return "Center";
				case UIHorizontalAlignment::Right:
					return "Right";
				case UIHorizontalAlignment::Fill:
					return "Fill";
				default:
					return "Left";
				}
			};

		if (ImGui::TreeNode(property.GetName().c_str()))
		{
			for (size_t i = 0; i < slots.size(); ++i)
			{
				HorizontalBoxSlot& slot = slots[i];
				ImGui::PushID(static_cast<int>(i));
				ImGui::Separator();

				std::string nameValue = slot.childName;
				if (slot.child && nameValue.empty())
				{
					nameValue = slot.child->GetName();
				}
				std::array<char, 256> nameBuffer{};
				CopyStringToBuffer(nameValue, nameBuffer);
				if (ImGui::InputText("Child", nameBuffer.data(), nameBuffer.size()))
				{
					slot.childName = nameBuffer.data();
					slot.child = nullptr;
					updated = true;
				}
				activated = activated || ImGui::IsItemActivated();
				deactivated = deactivated || ImGui::IsItemDeactivatedAfterEdit();

				float desiredValues[2] = { slot.desiredSize.width, slot.desiredSize.height };
				if (ImGui::DragFloat2("Desired Size", desiredValues, DRAG_SPEED))
				{
					slot.desiredSize.width = desiredValues[0];
					slot.desiredSize.height = desiredValues[1];
					updated = true;
				}
				activated = activated || ImGui::IsItemActivated();
				deactivated = deactivated || ImGui::IsItemDeactivatedAfterEdit();

				float paddingValues[4] = {
					slot.padding.left,
					slot.padding.right,
					slot.padding.top,
					slot.padding.bottom
				};

				if (ImGui::DragFloat4("Padding (L,R,T,B)", paddingValues, DRAG_SPEED))
				{
					slot.padding.left = paddingValues[0];
					slot.padding.right = paddingValues[1];
					slot.padding.top = paddingValues[2];
					slot.padding.bottom = paddingValues[3];
					updated = true;
				}
				activated = activated || ImGui::IsItemActivated();
				deactivated = deactivated || ImGui::IsItemDeactivatedAfterEdit();

				if (ImGui::DragFloat("Fill Weight", &slot.fillWeight, DRAG_SPEED))
				{
					updated = true;
				}
				activated = activated || ImGui::IsItemActivated();
				deactivated = deactivated || ImGui::IsItemDeactivatedAfterEdit();

				const char* alignmentPreview = alignmentLabel(slot.alignment);
				if (ImGui::BeginCombo("Alignment", alignmentPreview))
				{
					const UIHorizontalAlignment options[] = {
						UIHorizontalAlignment::Left,
						UIHorizontalAlignment::Center,
						UIHorizontalAlignment::Right,
						UIHorizontalAlignment::Fill
					};
					for (const auto& option : options)
					{
						const bool isSelected = (slot.alignment == option);
						if (ImGui::Selectable(alignmentLabel(option), isSelected))
						{
							slot.alignment = option;
							updated = true;
						}
						if (isSelected)
						{
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
				activated = activated || ImGui::IsItemActivated();
				deactivated = deactivated || ImGui::IsItemDeactivatedAfterEdit();

				if (ImGui::Button("Remove Slot"))
				{
					slots.erase(slots.begin() + static_cast<long>(i));
					updated = true;
					ImGui::PopID();
					break;
				}
				activated = activated || ImGui::IsItemActivated();
				deactivated = deactivated || ImGui::IsItemDeactivatedAfterEdit();

				ImGui::PopID();
			}

			if (ImGui::Button("Add Slot"))
			{
				slots.emplace_back();
				updated = true;
			}
			activated = activated || ImGui::IsItemActivated();
			deactivated = deactivated || ImGui::IsItemDeactivatedAfterEdit();

			ImGui::TreePop();
		}

		if (updated)
		{
			property.SetValue(component, &slots);
			result.updated = true;
		}
		result.activated = result.activated || activated;
		result.deactivated = result.deactivated || deactivated;
		return result;
	}

	if (typeInfo == typeid(std::vector<CanvasSlot>))
	{
		std::vector<CanvasSlot> slots;
		property.GetValue(component, &slots);
		bool updated = false;
		bool activated = false;
		bool deactivated = false;

		if (ImGui::TreeNode(property.GetName().c_str()))
		{
			for (size_t i = 0; i < slots.size(); ++i)
			{
				CanvasSlot& slot = slots[i];
				ImGui::PushID(static_cast<int>(i));
				ImGui::Separator();

				std::string nameValue = slot.childName;
				if (slot.child && nameValue.empty())
				{
					nameValue = slot.child->GetName();
				}
				std::array<char, 256> nameBuffer{};
				CopyStringToBuffer(nameValue, nameBuffer);
				if (ImGui::InputText("Child", nameBuffer.data(), nameBuffer.size()))
				{
					slot.childName = nameBuffer.data();
					slot.child = nullptr;
					updated = true;
				}
				activated = activated || ImGui::IsItemActivated();
				deactivated = deactivated || ImGui::IsItemDeactivatedAfterEdit();

				float rectValues[4] = { slot.rect.x, slot.rect.y, slot.rect.width, slot.rect.height };
				if (ImGui::DragFloat4("Rect", rectValues, DRAG_SPEED))
				{
					slot.rect.x = rectValues[0];
					slot.rect.y = rectValues[1];
					slot.rect.width = rectValues[2];
					slot.rect.height = rectValues[3];
					updated = true;
				}
				activated = activated || ImGui::IsItemActivated();
				deactivated = deactivated || ImGui::IsItemDeactivatedAfterEdit();

				if (ImGui::Button("Remove Slot"))
				{
					slots.erase(slots.begin() + static_cast<long>(i));
					updated = true;
					ImGui::PopID();
					break;
				}
				activated = activated || ImGui::IsItemActivated();
				deactivated = deactivated || ImGui::IsItemDeactivatedAfterEdit();

				ImGui::PopID();
			}

			if (ImGui::Button("Add Slot"))
			{
				slots.emplace_back();
				updated = true;
			}
			activated = activated || ImGui::IsItemActivated();
			deactivated = deactivated || ImGui::IsItemDeactivatedAfterEdit();

			ImGui::TreePop();
		}

		if (updated)
		{
			property.SetValue(component, &slots);
			result.updated = true;
		}
		result.activated = result.activated || activated;
		result.deactivated = result.deactivated || deactivated;
		return result;
	}

	if (typeInfo == typeid(UIStretch))
	{
		UIStretch value{};
		property.GetValue(component, &value);
		static constexpr const char* kStretchLabels[] = { "None", "Scale To Fit", "Scale To Fill" };
		int current = static_cast<int>(value);
		if (ImGui::Combo(property.GetName().c_str(), &current, kStretchLabels, IM_ARRAYSIZE(kStretchLabels)))
		{
			const int clamped = std::clamp(current, 0, static_cast<int>(IM_ARRAYSIZE(kStretchLabels) - 1));
			value = static_cast<UIStretch>(clamped);
			property.SetValue(component, &value);
			result.updated = true;
		}
		result.activated = result.activated || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		return result;
	}

	if (typeInfo == typeid(UIStretchDirection))
	{
		UIStretchDirection value{};
		property.GetValue(component, &value);
		static constexpr const char* kDirectionLabels[] = { "Both", "Down Only", "Up Only" };
		int current = static_cast<int>(value);
		if (ImGui::Combo(property.GetName().c_str(), &current, kDirectionLabels, IM_ARRAYSIZE(kDirectionLabels)))
		{
			const int clamped = std::clamp(current, 0, static_cast<int>(IM_ARRAYSIZE(kDirectionLabels) - 1));
			value = static_cast<UIStretchDirection>(clamped);
			property.SetValue(component, &value);
			result.updated = true;
		}
		result.activated = result.activated || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		return result;
	}

	if (typeInfo == typeid(UIFillDirection))
	{
		UIFillDirection value{};
		property.GetValue(component, &value);
		static constexpr const char* kDirectionLabels[] = { "Left To Right", "Right To Left", "Top To Bottom", "Bottom To Top" };
		int current = static_cast<int>(value);
		if (ImGui::Combo(property.GetName().c_str(), &current, kDirectionLabels, IM_ARRAYSIZE(kDirectionLabels)))
		{
			const int clamped = std::clamp(current, 0, static_cast<int>(IM_ARRAYSIZE(kDirectionLabels) - 1));
			value = static_cast<UIFillDirection>(clamped);
			property.SetValue(component, &value);
			result.updated = true;
		}
		result.activated = result.activated || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		return result;
	}

	if (typeInfo == typeid(UIProgressFillMode))
	{
		UIProgressFillMode value{};
		property.GetValue(component, &value);
		static constexpr const char* kFillModeLabels[] = { "Rect", "Mask" };
		int current = static_cast<int>(value);
		if (ImGui::Combo(property.GetName().c_str(), &current, kFillModeLabels, IM_ARRAYSIZE(kFillModeLabels)))
		{
			const int clamped = std::clamp(current, 0, static_cast<int>(IM_ARRAYSIZE(kFillModeLabels) - 1));
			value = static_cast<UIProgressFillMode>(clamped);
			property.SetValue(component, &value);
			result.updated = true;
		}
		result.activated = result.activated || ImGui::IsItemActivated();
		result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();
		return result;
	}


	if (typeInfo == typeid(std::array<EnemyMovementComponent::PatrolPoint, 3>))
	{
		std::array<EnemyMovementComponent::PatrolPoint, 3> points{};
		property.GetValue(component, &points);

		bool changed = false;

		ImGui::PushID(property.GetName().c_str());

		for (int i = 0; i < 3; ++i)
		{
			ImGui::SeparatorText(("PatrolPoint " + std::to_string(i)).c_str());

			ImGui::PushID(i);

			int q = points[i].q;
			int r = points[i].r;

			changed |= ImGui::InputInt("Q", &q);
			changed |= ImGui::InputInt("R", &r);

			if (changed)
			{
				points[i].q = q;
				points[i].r = r;
			}

			result.activated = result.activated || ImGui::IsItemActivated();
			result.deactivated = result.deactivated || ImGui::IsItemDeactivatedAfterEdit();

			ImGui::PopID();
		}

		ImGui::PopID();

		if (changed)
		{
			property.SetValue(component, &points);
			result.updated = true;
		}

		return result;
	}

	ImGui::TextDisabled("%s (Unsupported)", property.GetName().c_str());
	return result;
}

bool ProjectToViewport(
	const XMFLOAT3& world,
	const XMMATRIX& viewProj,
	const ImVec2& rectMin,
	const ImVec2& rectMax,
	ImVec2& out)
{
	const XMVECTOR projected = XMVector3TransformCoord(XMLoadFloat3(&world), viewProj);
	const float x = XMVectorGetX(projected);
	const float y = XMVectorGetY(projected);
	const float z = XMVectorGetZ(projected);

	if (z < 0.0f || z > 1.0f)
	{
		return false;
	}

	const ImVec2 rectSize = ImVec2(rectMax.x - rectMin.x, rectMax.y - rectMin.y);
	const float screenX = rectMin.x + (x * 0.5f + 0.5f) * rectSize.x;
	const float screenY = rectMin.y + (1.0f - (y * 0.5f + 0.5f)) * rectSize.y;
	out = ImVec2(screenX, screenY);
	return true;
}