
#include <cmath>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include "GridSystemComponent.h"
#include "TransformComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "Scene.h"
#include "NodeComponent.h"
#include "GameObject.h"
#include "PlayerComponent.h"
#include "PlayerMovementComponent.h"
#include "EnemyComponent.h"
#include "EnemyStatComponent.h"

REGISTER_COMPONENT(GridSystemComponent)
REGISTER_PROPERTY_READONLY(GridSystemComponent, NodesCount)


using namespace std;

// 면 갯수 / Index
constexpr int kNeighborCount = 6;
constexpr int kNeighborOffsets[kNeighborCount][2] = {
	{ 1, 0 },
	{ 1, -1 },
	{ 0, -1 },
	{ -1, 0 },
	{ -1, 1 },
	{ 0, 1 }
};
// unordered_map key로 사용할 것임

float ClampInnerRadius(float innerRadius)
{
	return (innerRadius > 0.0f) ? innerRadius : 1.0f;
}

float OuterRadiusFromInner(float innerRadius)
{
	return ClampInnerRadius(innerRadius) * 2.0f / std::sqrt(3.0f);
}

AxialCoord WorldToAxialPointy(const XMFLOAT3& pos, float innerRadius)
{
	const float size = OuterRadiusFromInner(innerRadius);
	const float invSize = (size > 0.0f) ? (1.0f / size) : 0.0f;
	const float x = pos.x;
	const float z = pos.z;
	return {
		(std::sqrt(3.0f) / 3.0f * x - 1.0f / 3.0f * z) * invSize,
		(2.0f / 3.0f * z) * invSize
	};
}

// HexaGrid 좌표의 정합성(3개의 합 0 유지) Cube -> Axial
AxialKey AxialRound(const AxialCoord& axial)
{
	CubeCoord cube{ axial.q, -axial.q - axial.r, axial.r };

	float rx = std::round(cube.x);
	float ry = std::round(cube.y);
	float rz = std::round(cube.z);

	const float dx = std::fabs(rx - cube.x);
	const float dy = std::fabs(ry - cube.y);
	const float dz = std::fabs(rz - cube.z);

	if (dx > dy && dx > dz)
	{
		rx = -ry - rz;
	}
	else if (dy > dz)
	{
		ry = -rx - rz;
	}
	else
	{
		rz = -rx - ry;
	}

	return { static_cast<int>(rx), static_cast<int>(rz) };
}
namespace
{
	int AxialDistance(int q1, int r1, int q2, int r2)
	{
		const int dq = q1 - q2;
		const int dr = r1 - r2;
		const int ds = dq + dr;
		return (std::abs(dq) + std::abs(dr) + std::abs(ds)) / 2;
	}

	CubeCoord AxialToCube(int q, int r)
	{
		return CubeCoord{ static_cast<float>(q), static_cast<float>(-q - r), static_cast<float>(r) };
	}

	CubeCoord CubeLerp(const CubeCoord& a, const CubeCoord& b, float t)
	{
		return CubeCoord{
			a.x + (b.x - a.x) * t,
			a.y + (b.y - a.y) * t,
			a.z + (b.z - a.z) * t
		};
	}

	AxialKey CubeRound(const CubeCoord& cube)
	{
		float rx = std::round(cube.x);
		float ry = std::round(cube.y);
		float rz = std::round(cube.z);

		const float dx = std::fabs(rx - cube.x);
		const float dy = std::fabs(ry - cube.y);
		const float dz = std::fabs(rz - cube.z);

		if (dx > dy && dx > dz)
		{
			rx = -ry - rz;
		}
		else if (dy > dz)
		{
			ry = -rx - rz;
		}
		else
		{
			rz = -rx - ry;
		}

		return { static_cast<int>(rx), static_cast<int>(rz) };
	}
}

GridSystemComponent::GridSystemComponent() = default;

GridSystemComponent::~GridSystemComponent() {
}

void GridSystemComponent::Start()
{

	ScanNodes();
	MakeGraph();
}

void GridSystemComponent::Update(float deltaTime) {

	UpdateActorPositions();
	const int moveRange = m_Player ? m_Player->GetRemainMoveResource() : 0; // 남은 이동 자원 Get

	bool isDragging = false;
	NodeComponent* rangeStartNode = m_PlayerNode;
	if (m_Player)
	{
		auto* playerOwner = m_Player->GetOwner();
		if (playerOwner)
		{
			if (auto* movement = playerOwner->GetComponent<PlayerMovementComponent>())
			{
				isDragging = movement->IsDragging();
				if (isDragging && movement->GetDragStartNode())
				{
					rangeStartNode = movement->GetDragStartNode();
				}
			}
		}
	}

	UpdateMoveRange(rangeStartNode, moveRange);

	if (isDragging && moveRange > 0)
	{
		m_MoveRangePulseTime += deltaTime;
		const float pulseSpeed = 2.0f + static_cast<float>(moveRange) * 0.5f;
		const float pulse = 0.5f + 0.5f * std::sin(m_MoveRangePulseTime * pulseSpeed);
		UpdateMoveRangeMaterials(pulse, true);
	}
	else
	{
		m_MoveRangePulseTime = 0.0f;
		UpdateMoveRangeMaterials(0.0f, false);
	}
	if (m_ThrowRangePreviewActive && m_ThrowRange > 0)
	{
		UpdateThrowRange(m_PlayerNode, m_ThrowRange);
		m_ThrowRangePulseTime += deltaTime;
		const float pulseSpeed = 2.0f + static_cast<float>(m_ThrowRange) * 0.5f;
		const float pulse = 0.5f + 0.5f * std::sin(m_ThrowRangePulseTime * pulseSpeed);
		UpdateThrowRangeMaterials(pulse, true);
	}
	else
	{
		m_ThrowRangePulseTime = 0.0f;
		UpdateThrowRange(nullptr, 0);
		UpdateThrowRangeMaterials(0.0f, false);
	}
}

void GridSystemComponent::OnEvent(EventType type, const void* data)
{
	(void)type;
	(void)data;
}

void GridSystemComponent::SetThrowRangePreview(bool enabled, int range)
{
	m_ThrowRangePreviewActive = enabled;
	m_ThrowRange = max(0, range);

	if (!enabled)
	{
		m_ThrowRangePulseTime = 0.0f;
		UpdateThrowRange(nullptr, 0);
		UpdateThrowRangeMaterials(0.0f, false);
	}
}

//Node object, player, Enemy 찾아서 배정
void GridSystemComponent::ScanNodes()
{
	m_NodesCount = 0;
	m_Nodes.clear();
	m_NodesByAxial.clear();
	m_Player = nullptr;
	m_Enemies.clear();


	if (!GetOwner()) { return; }
	auto* scene = GetOwner()->GetScene();
	if (!scene) { return; }

	const auto& objects = scene->GetGameObjects();

	// 메모리를 불필요하게 잡는 것일 수 있음 필요하면 최적화
	m_Nodes.reserve(objects.size()); 
	m_NodesByAxial.reserve(objects.size());

	int nextEnemyId = 2;
	//등록 과정 ( Transform 기반 Axial 좌표변환 후 등록) ** Enemy, Player 도 추가
	for (const auto& [name, object] : objects) {
		if(!object){ continue;}

		auto* node = object->GetComponent<NodeComponent>();

		if(!node) { 
			auto* enemy =  object->GetComponent<EnemyComponent>();
			if (!enemy) {
				//Player 처리
				auto* player = object->GetComponent<PlayerComponent>();
				if (!player) { continue; }
				auto* trans = object->GetComponent<TransformComponent>();
				const AxialKey axial = AxialRound(WorldToAxialPointy(trans->GetPosition(), m_InnerRadius));
				player->SetQR(axial.q, axial.r);
				player->SetActorId(1);
				m_Player = player;
				continue;
			}
			//Enemy 처리
			auto* trans = object->GetComponent<TransformComponent>();
			const AxialKey axial = AxialRound(WorldToAxialPointy(trans->GetPosition(), m_InnerRadius));
			enemy->SetQR(axial.q, axial.r);
			enemy->SetActorId(nextEnemyId++);
			m_Enemies.push_back(enemy);
			continue;
		}

		// node처리
		auto* trans = object->GetComponent<TransformComponent>();
		const AxialKey axial = AxialRound(WorldToAxialPointy(trans->GetPosition(), m_InnerRadius));


		node->SetQR(axial.q, axial.r);
		node->SetState(NodeState::Empty); //?

		m_Nodes.push_back(node);
		m_NodesByAxial[{ axial.q, axial.r }] = node;

		m_NodesCount++; // debuging 용ㅤ
	}

	if (m_Player) {
		const AxialKey current{ m_Player->GetQ(), m_Player->GetR() };
		UpdateActorNodeState(current, current, NodeState::HasPlayer);
	}

	for (auto* enemy : m_Enemies) {
		if (!enemy) {
			continue;
		}
		const AxialKey current{ enemy->GetQ(), enemy->GetR() };
		UpdateActorNodeState(current, current, NodeState::HasEnemy);
	}

}

int GridSystemComponent::GetShortestPathLength(const AxialKey& start, const AxialKey& target)
{
	auto* startNode = GetNodeByKey(start);
	auto* targetNode = GetNodeByKey(target);

	if (!startNode || !targetNode)
	{
		return -1;
	}
	if (startNode == targetNode)
	{
		return 0;
	}
	const PathResult result = PathBFS(startNode, targetNode);
	auto it = result.distances.find(targetNode);
	if (it == result.distances.end())
	{
		return -1;
	}
	return it->second;

}

std::vector<AxialKey> GridSystemComponent::GetShortestPath(const AxialKey& start, const AxialKey& target) const
{

	auto* startNode = GetNodeByKey(start);
	auto* targetNode = GetNodeByKey(target);

	if (!startNode || !targetNode) { return{}; }
	if (startNode == targetNode) { return{ start }; }

	const PathResult result = PathBFS(startNode, targetNode);
	auto it = result.cameFrom.find(targetNode);
	if (it == result.cameFrom.end()) { return{}; }

	std::vector<AxialKey> path;
	for (auto* current = targetNode; current; current = result.cameFrom.at(current)) {
		path.push_back({ current->GetQ(),current->GetR() }); 
	}
	std::reverse(path.begin(), path.end());
	return path;

}

// Axial 좌표로 Node 찾기
NodeComponent* GridSystemComponent::GetNodeByKey(const AxialKey& key) const
{
	auto it = m_NodesByAxial.find(key);
	if (it == m_NodesByAxial.end())
	{
		return nullptr;
	}
	return it->second;
}

EnemyComponent* GridSystemComponent::GetEnemyAt(int q, int r) const
{
	for (auto* enemy : m_Enemies)
	{
		auto* enemyOwner = enemy->GetOwner();
		auto* enemyStat = enemyOwner ? enemyOwner->GetComponent<EnemyStatComponent>() : nullptr;
		if (enemyStat && enemyStat->IsDead())
		{
			continue;
		}

		if (enemy && enemy->GetQ() == q && enemy->GetR() == r)
			return enemy;
	}
	return nullptr;
}

// 최초 위치에서 이동가능한 범위 표시
void GridSystemComponent::UpdateMoveRange(NodeComponent* startNode, int range)
{
	for (auto* node : m_Nodes)
	{
		if (!node)
		{
			continue;
		}
		node->SetInMoveRange(false);
	}

	if (!startNode || range <= 0)
	{
		return;
	}

	std::unordered_map<NodeComponent*, int> distances;
	std::queue<NodeComponent*> frontier;

	distances[startNode] = 0;
	frontier.push(startNode);
	startNode->SetInMoveRange(true);

	while (!frontier.empty())
	{
		NodeComponent* current = frontier.front();
		frontier.pop();

		const int currentDistance = distances[current];
		if (currentDistance >= range)
		{
			continue;
		}

		for (auto* neighbor : current->GetNeighbors())
		{
			if (!neighbor)
			{
				continue;
			}

			if (neighbor->GetState() != NodeState::Empty)
			{
				continue;
			}

			if (!neighbor->GetIsMoveable())
			{
				continue;
			}

			if (distances.find(neighbor) != distances.end())
			{
				continue;
			}

			const int nextDistance = currentDistance + 1;
			distances[neighbor] = nextDistance;
			neighbor->SetInMoveRange(true);
			frontier.push(neighbor);
		}
	}
}

void GridSystemComponent::UpdateMoveRangeMaterials(float pulse, bool enabled)
{
	for (auto* node : m_Nodes)
	{
		if (!node)
		{
			continue;
		}

		if (enabled && node->IsInMoveRange())
		{
			node->SetMoveRangeHighlight(pulse, true);
		}
		else
		{
			node->SetMoveRangeHighlight(0.0f, false);
		}
	}
}

void GridSystemComponent::UpdateThrowRange(NodeComponent* startNode, int range)
{
	for (auto* node : m_Nodes)
	{
		if (!node)
		{
			continue;
		}

		node->SetInThrowRange(false);
	}

	if (!startNode || range <= 0)
	{
		return;
	}

	for (auto* node : m_Nodes)
	{
		if (!node || !node->GetIsMoveable())
		{
			continue;
		}

		const int distance = AxialDistance(startNode->GetQ(), startNode->GetR(), node->GetQ(), node->GetR());
		if (distance <= range && HasClearSightLine(startNode->GetQ(), startNode->GetR(), node->GetQ(), node->GetR())) 
		{
			node->SetInThrowRange(true);
		}
	}
}

bool GridSystemComponent::HasClearSightLine(int fromQ, int fromR, int toQ, int toR) const
{
	if (fromQ == toQ && fromR == toR)
	{
		return true;
	}

	const int distance = AxialDistance(fromQ, fromR, toQ, toR);
	if (distance <= 0)
	{
		return true;
	}

	const CubeCoord startCube = AxialToCube(fromQ, fromR);
	const CubeCoord endCube = AxialToCube(toQ, toR);

	for (int step = 1; step < distance; ++step)
	{
		const float t = static_cast<float>(step) / static_cast<float>(distance);
		const AxialKey key = CubeRound(CubeLerp(startCube, endCube, t));
		auto it = m_NodesByAxial.find(key);
		if (it == m_NodesByAxial.end() || !it->second)
		{
			return false;
		}

		if (!it->second->GetIsSight())
		{
			return false;
		}
	}

	return true;
}

void GridSystemComponent::UpdateThrowRangeMaterials(float pulse, bool enabled)
{
	for (auto* node : m_Nodes)
	{
		if (!node)
		{
			continue;
		}

		if (enabled && node->IsInThrowRange())
		{
			node->SetThrowRangeHighlight(pulse, true);
		}
		else
		{
			node->SetThrowRangeHighlight(0.0f, false);
		}
	}
}

//Player/ Enemy 변경에 따른 Node state Update
void GridSystemComponent::UpdateActorPositions() 
{
	std::unordered_set<AxialKey, AxialKeyHash> occupiedByEnemy;
	occupiedByEnemy.reserve(m_Enemies.size());

	if (m_Player) {
		bool skipPlayerUpdate = false;
		auto* playerOwner = m_Player->GetOwner();
		if (playerOwner)
		{
			if (auto* movement = playerOwner->GetComponent<PlayerMovementComponent>())
			{
				skipPlayerUpdate = movement->IsDragging();
			}
		}
		if (!skipPlayerUpdate)
		{
			auto* trans = playerOwner ? playerOwner->GetComponent<TransformComponent>() : nullptr;
			if (trans) {
				const AxialKey previous{ m_Player->GetQ(), m_Player->GetR() };
				const AxialKey current = AxialRound(WorldToAxialPointy(trans->GetPosition(), m_InnerRadius));
				if (!(previous == current)) {
					UpdateActorNodeState(previous, current, NodeState::HasPlayer);
					m_Player->SetQR(current.q, current.r);
				}
				else
				{
					auto it = m_NodesByAxial.find(current);
					if (it != m_NodesByAxial.end() && it->second)
					{
						it->second->SetState(NodeState::HasPlayer);
						m_PlayerNode = it->second;
					}
				}
			}
		}
	}

	for (auto* enemy : m_Enemies) {
		if (!enemy) {
			continue;
		}
		auto* enemyOwner = enemy->GetOwner();
		auto* trans = enemyOwner ? enemyOwner->GetComponent<TransformComponent>() : nullptr;
		if (!trans) {
			const AxialKey previous{ enemy->GetQ(), enemy->GetR() };
			auto it = m_NodesByAxial.find(previous);
			if (it != m_NodesByAxial.end() && it->second && it->second->GetState() == NodeState::HasEnemy)
			{
				it->second->SetState(NodeState::Empty);
			}
			continue;
		}
		const AxialKey previous{ enemy->GetQ(), enemy->GetR() };
		
		auto* enemyStat = enemyOwner ? enemyOwner->GetComponent<EnemyStatComponent>() : nullptr;
		if (enemyStat && enemyStat->IsDead())
		{
			auto it = m_NodesByAxial.find(previous);
			if (it != m_NodesByAxial.end() && it->second && it->second->GetState() == NodeState::HasEnemy)
			{
				it->second->SetState(NodeState::Empty);
			}
			continue;
		}

		const AxialKey current = AxialRound(WorldToAxialPointy(trans->GetPosition(), m_InnerRadius));
		if (!(previous == current)) {
			UpdateActorNodeState(previous, current, NodeState::HasEnemy);
			enemy->SetQR(current.q, current.r);
			occupiedByEnemy.insert(current);
		}
		else
		{
			auto it = m_NodesByAxial.find(current);
			if (it != m_NodesByAxial.end() && it->second)
			{
				it->second->SetState(NodeState::HasEnemy);
				occupiedByEnemy.insert(current);
			}
		}
	}
	for (auto* node : m_Nodes)
	{
		if (!node)
		{
			continue;
		}
		if (node->GetState() != NodeState::HasEnemy)
		{
			continue;
		}

		const AxialKey key{ node->GetQ(), node->GetR() };
		if (occupiedByEnemy.find(key) == occupiedByEnemy.end())
		{
			node->SetState(NodeState::Empty);
		}
	}
}

void GridSystemComponent::UpdateActorNodeState(const AxialKey& previous, const AxialKey& current, NodeState state)
{
	if (!(previous == current)) {
		auto it = m_NodesByAxial.find(previous);
		if (it != m_NodesByAxial.end() && it->second) {
			if (it->second->GetState() == state) {
				it->second->SetState(NodeState::Empty);
			}
		}
	}

	auto it = m_NodesByAxial.find(current);
	if (it != m_NodesByAxial.end() && it->second) {
		it->second->SetState(state);
		if (state == NodeState::HasPlayer) {
			m_PlayerNode = it->second;
		}
	}
	else if (state == NodeState::HasPlayer) {
		m_PlayerNode = nullptr;
	}
}


GridSystemComponent::PathResult GridSystemComponent::PathBFS(const NodeComponent* startNode, const NodeComponent* targetNode) const
{
	PathResult result;
	if (!startNode || !targetNode) { return result; }

	std::queue<NodeComponent*> frontier;
	frontier.push(const_cast<NodeComponent*>(startNode));
	result.cameFrom[const_cast<NodeComponent*>(startNode)] = nullptr;
	result.distances[const_cast<NodeComponent*>(startNode)] = 0;

	while (!frontier.empty())
	{
		auto* current = frontier.front();
		frontier.pop();

		if (current == targetNode)
		{
			break;
		}

		const int currentDistance = result.distances[current];
		for (auto* neighbor : current->GetNeighbors())
		{
			if (!neighbor)
			{
				continue;
			}
			if (!neighbor->GetIsMoveable())
			{
				continue;
			}
			if (neighbor->GetState() != NodeState::Empty && neighbor != targetNode)
			{
				continue;
			}
			if (result.distances.find(neighbor) != result.distances.end())
			{
				continue;
			}

			result.cameFrom[neighbor] = current;
			result.distances[neighbor] = currentDistance + 1;
			frontier.push(neighbor);
		}
	}

	return result;
}

void GridSystemComponent::MakeGraph()
{
	//unordered_map<AxialKey, NodeComponent*, AxialKeyHash> nodesByAxial;

	for (auto* node : m_Nodes)
	{
		if (!node)
		{
			continue;
		}

		node->ClearNeighbors();
	}

	// 모든 노드들에 대하여 인접 노드 등록 (Hash된 QRKey 로)
	for (auto* node : m_Nodes) {

		const int q = node->GetQ();
		const int r = node->GetR();

		for (int i = 0; i < kNeighborCount; i++) {

			const AxialKey key{ q + kNeighborOffsets[i][0], r + kNeighborOffsets[i][1] };

			auto it = m_NodesByAxial.find(key);
			if (it != m_NodesByAxial.end()) {
				node->AddNeighbor(it->second);
			}
		}
	}

}

