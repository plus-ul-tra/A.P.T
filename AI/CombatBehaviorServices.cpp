#include "CombatBehaviorServices.h"
#include "Blackboard.h"
#include "BlackboardKeys.h"
#include <iostream>
#include <cmath>
#include "EventDispatcher.h"
#include "IEventListener.h"
#include "CombatEvents.h"

bool TryGetFloat(Blackboard& bb, const char* key, float& out)
{
	return bb.TryGet(key, out);
}

bool TryGetInt(Blackboard& bb, const char* key, int& out)
{
	return bb.TryGet(key, out);
}

float Clamp(float value, float minValue, float maxValue)
{
	if (value < minValue)
		return minValue;
	if (value > maxValue)
		return maxValue;

	return value;
}

void TargetSenseService::TickService(BTInstance& inst, Blackboard& bb, float deltaTime)
{
	(void)inst;
	(void)deltaTime;

	int selfQ = 0;
	int selfR = 0;
	int targetQ = 0;
	int targetR = 0;
	bool hasHexSightData = false;
	bool hasTargetHexLine = false;

	bool isInCombat = false;
	bb.TryGet(BlackboardKeys::IsInCombat, isInCombat);

	/*const bool hasHexData = TryGetInt(bb, BlackboardKeys::SelfQ, selfQ)
		&& TryGetInt(bb, BlackboardKeys::SelfR, selfR)
		&& TryGetInt(bb, BlackboardKeys::TargetQ, targetQ)
		&& TryGetInt(bb, BlackboardKeys::TargetR, targetR);*/
	const bool hasSelfData = TryGetInt(bb, BlackboardKeys::SelfQ, selfQ)
		&& TryGetInt(bb, BlackboardKeys::SelfR, selfR);
	bool hasTargetData = TryGetInt(bb, BlackboardKeys::TargetQ, targetQ)
		&& TryGetInt(bb, BlackboardKeys::TargetR, targetR);

	if (!hasTargetData && isInCombat)
	{
		hasTargetData = TryGetInt(bb, BlackboardKeys::LastKnownTargetQ, targetQ)
			&& TryGetInt(bb, BlackboardKeys::LastKnownTargetR, targetR);
	}

	const bool hasHexData = hasSelfData && hasTargetData;

	if (!hasHexData)
	{
		if (isInCombat)
		{
			bb.Set(BlackboardKeys::HasTarget, true);
		}
		else
		{
			bb.Set(BlackboardKeys::HasTarget, false);
		}
		return;
	}

	const int dq = selfQ - targetQ;
	const int dr = selfR - targetR;
	const int ds = dq + dr;
	const float distance = 0.5f * static_cast<float>(std::abs(dq) + std::abs(dr) + std::abs(ds));
	float meleeRange = 1.0f;
	bb.TryGet(BlackboardKeys::MeleeRange, meleeRange);

	bb.Set(BlackboardKeys::TargetDistance, distance);
	bb.Set(BlackboardKeys::TargetAngle, 0.0f);

	if (isInCombat)
	{
		bb.Set(BlackboardKeys::HasTarget, true);
		return;
	}

	const bool useHexSight = bb.TryGet(BlackboardKeys::HasHexSightData, hasHexSightData)
		&& hasHexSightData
		&& bb.TryGet(BlackboardKeys::HasTargetHexLine, hasTargetHexLine);

	const bool inMeleeRange = distance <= meleeRange;
	bb.Set(BlackboardKeys::HasTarget, inMeleeRange || (useHexSight && hasTargetHexLine));
}

void CombatStateSyncService::TickService(BTInstance& inst, Blackboard& bb, float deltaTime)
{
	(void)inst;
	(void)deltaTime;
	bool hasTarget = false;
	bb.TryGet(BlackboardKeys::HasTarget, hasTarget);

	bool isBattleActor = false;
	bb.TryGet(BlackboardKeys::IsBattleActor, isBattleActor);

	if (hasTarget && isBattleActor)
	{
		bb.Set(BlackboardKeys::IsInCombat, true);
	}
}

void RangeUpdateService::TickService(BTInstance& inst, Blackboard& bb, float deltaTime)
{
	(void)inst;
	(void)deltaTime;

	float distance = 0.0f;
	float meleeRange = 1.0f;
	float throwRange = 3.0f;

	bb.TryGet(BlackboardKeys::TargetDistance, distance);
	bb.TryGet(BlackboardKeys::MeleeRange, meleeRange);
	bb.TryGet(BlackboardKeys::ThrowRange, throwRange);

	bb.Set(BlackboardKeys::InMeleeRange, distance <= meleeRange);
	bb.Set(BlackboardKeys::InThrowRange, distance <= throwRange);

	// 추가: 원거리 선호면 MaintainRange 켜기
	bool preferRanged = false;
	bb.TryGet(BlackboardKeys::PreferRanged, preferRanged);
	bb.Set(BlackboardKeys::MaintainRange, preferRanged);
}

void EstimatePlayerDamageService::TickService(BTInstance& inst, Blackboard& bb, float deltaTime)
{
	(void)inst;
	(void)deltaTime;

	int maxDamage = 0;
	if (!bb.TryGet(BlackboardKeys::PlayerMaxDamage, maxDamage))
	{
		maxDamage = 1;
	}

	int hp = 0;
	if (!bb.TryGet(BlackboardKeys::HP, hp))
	{
		hp = 30;
	}

	bb.Set(BlackboardKeys::EstimatedPlayerDamage, maxDamage);
	bb.Set(BlackboardKeys::ShouldRunOff, maxDamage >= hp);
}

void RepathService::TickService(BTInstance& inst, Blackboard& bb, float deltaTime)
{
	(void)inst;
	(void)deltaTime;

	bool hasTarget = false;
	bb.TryGet(BlackboardKeys::HasTarget, hasTarget);
	bb.Set(BlackboardKeys::PathNeedsUpdate, hasTarget);
}

AIRequestDispatchService::AIRequestDispatchService(EventDispatcher* dispatcher)
	: m_Dispatcher(dispatcher)
{
}

void AIRequestDispatchService::TickService(BTInstance& inst, Blackboard& bb, float deltaTime)
{
	(void)inst;
	(void)deltaTime;

	if (!m_Dispatcher)
		return;

	bool isInCombat = false;
	bb.TryGet(BlackboardKeys::IsInCombat, isInCombat);
	bool isBattleActor = false;
	bb.TryGet(BlackboardKeys::IsBattleActor, isBattleActor);

	// 전투 참여자가 아니면 글로벌 전투 이벤트(AIMoveRequested 등)를 쏘지 않는다.
	// 비참여 적은 EnemyComponent에서 블랙보드 요청을 직접 소비해 순찰 이동한다.
	if (isInCombat && isBattleActor)
	{
		// 1) Move
		bool moveRequested = false;
		if (bb.TryGet(BlackboardKeys::MoveRequested, moveRequested) && moveRequested)
		{
			m_Dispatcher->Dispatch(EventType::AIMoveRequested, nullptr);
			bb.Set(BlackboardKeys::MoveRequested, false);
		}

		// 2) RunOff Move
		bool runOffMoveRequested = false;
		if (bb.TryGet(BlackboardKeys::RequestRunOffMove, runOffMoveRequested) && runOffMoveRequested)
		{
			m_Dispatcher->Dispatch(EventType::AIRunOffMoveRequested, nullptr);
			bb.Set(BlackboardKeys::RequestRunOffMove, false);
		}

		// 3) Maintain Range
		bool maintainRangeRequested = false;
		if (bb.TryGet(BlackboardKeys::RequestMaintainRange, maintainRangeRequested) && maintainRangeRequested)
		{
			m_Dispatcher->Dispatch(EventType::AIMaintainRangeRequested, nullptr);
			bb.Set(BlackboardKeys::RequestMaintainRange, false);
		}

		bool meleeRequested = false;
		if (bb.TryGet(BlackboardKeys::RequestMeleeAttack, meleeRequested) && meleeRequested)
		{
			//m_Dispatcher->Dispatch(EventType::AIMeleeAttackRequested, nullptr);
			int actorId = 0;
			bb.TryGet(BlackboardKeys::ActorId, actorId);
			const CombatAIRequestEvent eventData{ actorId };
			m_Dispatcher->Dispatch(EventType::AIMeleeAttackRequested, &eventData);
			bb.Set(BlackboardKeys::RequestMeleeAttack, false);
		}

		bool rangedRequested = false;
		if (bb.TryGet(BlackboardKeys::RequestRangedAttack, rangedRequested) && rangedRequested)
		{
			//m_Dispatcher->Dispatch(EventType::AIRangedAttackRequested, nullptr);
			int actorId = 0;
			bb.TryGet(BlackboardKeys::ActorId, actorId);
			const CombatAIRequestEvent eventData{ actorId };
			m_Dispatcher->Dispatch(EventType::AIRangedAttackRequested, &eventData);
			bb.Set(BlackboardKeys::RequestRangedAttack, false);
		}
	}

	// 4) End Turn
	bool endTurnRequested = false;
	if (bb.TryGet(BlackboardKeys::EndTurnRequested, endTurnRequested) && endTurnRequested)
	{
		if (isInCombat)
		{
			m_Dispatcher->Dispatch(EventType::AITurnEndRequested, nullptr);
		}
		else
		{
			bb.Set(BlackboardKeys::ExploreEndTurnRequested, true);
		}
		bb.Set(BlackboardKeys::EndTurnRequested, false);
	}
}
