#pragma once

/// 만들기 귀찮아서 만들어둔 Template(c++ Template 아님)
/// 복붙해서 이름 바꿔서 쓰세요
#include "Component.h"

// 이벤트 리스너는 쓸 얘들만
class EmptyComponent : public Component, public IEventListener {
	friend class Editor;
public:
	static constexpr const char* StaticTypeName = "EmptyComponent";
	const char* GetTypeName() const override;

	EmptyComponent();
	virtual ~EmptyComponent();

	void Start() override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override; // IEventListener 필요

private:


};