#pragma once

namespace BlackboardKeys
{
	inline constexpr const char* IsAlive			     = "IsAlive";
	inline constexpr const char* HasTarget               = "HasTarget";
	inline constexpr const char* IsInCombat              = "IsInCombat";
	inline constexpr const char* IsBattleActor		     = "IsBattleActor";
	inline constexpr const char* ShouldRunOff		     = "ShouldRunOff";
	inline constexpr const char* PreferRanged            = "PreferRanged";
	inline constexpr const char* MaintainRange           = "MaintainRange";
	inline constexpr const char* TargetDistance          = "TargetDistance";
	inline constexpr const char* TargetAngle             = "TargetAngle";
	inline constexpr const char* InMeleeRange            = "InMeleeRange";
	inline constexpr const char* InThrowRange            = "InThrowRange";
	inline constexpr const char* EstimatedPlayerDamage   = "EstimatedPlayerDamage";
	inline constexpr const char* PathNeedsUpdate	     = "PathNeedsUpdate";
	inline constexpr const char* MoveRequested		     = "MoveRequested";
	inline constexpr const char* RequestMeleeAttack      = "RequestMeleeAttack";
	inline constexpr const char* RequestRangedAttack     = "RequestRangedAttack";
	inline constexpr const char* RequestRunOffMove	     = "RequestRunOffMove";
	inline constexpr const char* RequestMaintainRange    = "RequestMaintainRange";
	inline constexpr const char* EndTurnRequested        = "EndTurnRequested";
	inline constexpr const char* EndTurnDelay		     = "EndTurnDelay";
	inline constexpr const char* ExploreEndTurnRequested = "ExploreEndTurnRequested";
	inline constexpr const char* RunOffTargetFound       = "RunOffTargetFound";
	inline constexpr const char* SelfQ				     = "SelfQ";
	inline constexpr const char* SelfR				     = "SelfR";
	inline constexpr const char* ActorId			     = "ActorId";
	inline constexpr const char* TargetQ			     = "TargetQ";
	inline constexpr const char* TargetR			     = "TargetR";
	inline constexpr const char* FacingDirection	     = "FacingDirection";
	inline constexpr const char* SightDistance           = "SightDistance";
	inline constexpr const char* SightAngle		         = "SightAngle";
	inline constexpr const char* HasHexSightData         = "HasHexSightData";
	inline constexpr const char* HasTargetHexLine        = "HasTargetHexLine";
	inline constexpr const char* MeleeRange		         = "MeleeRange";
	inline constexpr const char* ThrowRange		         = "ThrowRange";
	inline constexpr const char* PlayerMaxDamage         = "PlayerMaxDamage";
	inline constexpr const char* LastKnownTargetQ	     = "LastKnownTargetQ";
	inline constexpr const char* LastKnownTargetR	     = "LastKnownTargetR";
	inline constexpr const char* HP					     = "HP";
}