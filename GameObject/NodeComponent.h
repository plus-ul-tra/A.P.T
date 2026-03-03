#pragma once
#include <vector>
#include "Component.h"
#include "MathHelper.h"
#include "RenderData.h"

class MaterialComponent;
class DoorComponent;

using namespace MathUtils;
using namespace std; 
// 노드 상태
enum class NodeState {
	Empty,
	HasPlayer,
	HasEnemy,
	//HasObstacle, -> m_IsMoveable = false 로 고정 설정 할것이기 때문에 제외 
};


class NodeComponent : public Component, public IEventListener {
	friend class Editor;
public:
	static constexpr const char* StaticTypeName = "NodeComponent";
	const char* GetTypeName() const override;

	NodeComponent();
	virtual ~NodeComponent();

	void Start() override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void SetIsMoveable(const bool& is) { m_IsMoveable = is; };
	const bool& GetIsMoveable() const { return m_IsMoveable; }

	void SetIsSight(const bool& is) { m_IsSight = is; }
	const bool& GetIsSight() const { return m_IsSight; }

	void SetQR(int q, int r) { m_Q = q; m_R = r; }


	const int& GetQ() const { return m_Q; }
	const int& GetR() const { return m_R; }

	//상태 발판 설정
	void SetState(NodeState state) { m_State = state; }
	NodeState GetState() const { return m_State; }

	const int& GetStateInt() const { return m_StateInt; } // Debug

	void ClearNeighbors();
	void AddNeighbor(NodeComponent* node);
	const vector<NodeComponent*>& GetNeighbors() const { return m_Neighbors; }

	void SetInMoveRange(bool isInMoveRange) { m_IsInMoveRange = isInMoveRange; }
	bool IsInMoveRange() const { return m_IsInMoveRange; }
	void SetInThrowRange(bool isInThrowRange) { m_IsInThrowRange = isInThrowRange; }
	bool IsInThrowRange() const { return m_IsInThrowRange; }
	void SetMoveRangeHighlight(float intensity, bool enabled);
	void SetThrowRangeHighlight(float intensity, bool enabled);
	void SetSightHighlight(float intensity, bool enabled);
	void ClearHighlights();

	void SetLinkedDoorName(const std::string& name) { m_LinkedDoorName = name; }
	const std::string& GetLinkedDoorName() const { return m_LinkedDoorName; }
	DoorComponent* GetLinkedDoor() const { return m_LinkedDoor; }
	void SetLinkedDoor(DoorComponent* door, const std::string& doorName);

private:
	void ApplyHighlight();
	void ResolveLinkedDoor(); //Door 연결


	bool m_IsMoveable = true;	  //장애물 있으면 Editor에서 배치할때 false로 설정하기
	bool m_IsSight = true; // 적 시야 판별 / false = 적 시야가 넘어가서 볼 수 없음 (벽 타일)
	bool m_IsInMoveRange = false; // 이동가능 범위에 있는지
	bool m_IsInThrowRange = false;	//던지는 범위에 있는지
	//Read Only Property
	NodeState m_State = NodeState::Empty;
	bool m_UsingMoveRangeHighlight = false;
	bool m_UsingThrowRangeHighlight = false;

	bool m_UsingSightHighlight = false;
	float m_MoveHighlightIntensity = 0.0f;
	float m_ThrowHighlightIntensity = 0.0f;
	float m_SightHighlightIntensity = 0.0f;

	bool m_HasBaseMaterial = false;
	RenderData::MaterialData m_BaseMaterialOverrides{};
	MaterialComponent* m_Material = nullptr; 
	/*bool m_HasLastAppliedOverrides = false;
	RenderData::MaterialData m_LastAppliedOverrides{};*/
	int m_Q; 
	int m_R;
	int m_StateInt = 0; // Debug
	std::string m_LinkedDoorName;
	DoorComponent* m_LinkedDoor = nullptr;
	// Property X
	vector<NodeComponent*> m_Neighbors;
};