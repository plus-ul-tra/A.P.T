/// 만들기 귀찮아서 만들어둔 Template(c++ Template 아님)
/// 복붙해서 쓰세요
#include "ComponentTemplate.h"
#include "ReflectionMacro.h"
#include "Object.h"

REGISTER_COMPONENT(EmptyComponent)

EmptyComponent::EmptyComponent() {

}

EmptyComponent::~EmptyComponent() {
	// Event Listener 쓰는 경우만
	GetEventDispatcher().RemoveListener(EventType::KeyDown, this);
	GetEventDispatcher().RemoveListener(EventType::KeyUp, this);
}

void EmptyComponent::Start()
{
	// 이벤트 추가
	GetEventDispatcher().AddListener(EventType::KeyDown, this); 
	GetEventDispatcher().AddListener(EventType::KeyUp, this);
}

void EmptyComponent::Update(float deltaTime) {


}