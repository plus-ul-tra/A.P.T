#pragma once
#include "Object.h"
#include "json.hpp"

class TransformComponent;

class GameObject : public Object 
{
	friend class Editor;
	friend class Scene;

public:
	GameObject(EventDispatcher& eventDispatcher);
	virtual ~GameObject() = default;

	using Object::AddComponent;

	void AddComponent(std::unique_ptr<Component> comp)	//Deserialize용
	{
		if (!CanAddComponent(typeid(*comp)))
			return;

		comp->SetOwner(this);
		comp->Start();
		m_Components[comp->GetTypeName()].emplace_back(std::move(comp));
	}

	//bool IsInView(CameraObject* camera) const;

	//virtual void Render(std::vector<RenderInfo>& renderInfo);


	virtual void Serialize(nlohmann::json& j) const;
	virtual void Deserialize(const nlohmann::json& j);

	GameObject           (const GameObject&) = delete;
	GameObject& operator=(const GameObject&) = delete;
	GameObject           (GameObject&&)      = delete;
	GameObject& operator=(GameObject&&)      = delete;

protected:
	TransformComponent* m_Transform;
};

