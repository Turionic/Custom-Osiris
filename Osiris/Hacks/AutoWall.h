#pragma once
#include "../SDK/GlobalVars.h"
#include "../SDK/EngineTrace.h"

struct FireBulletData
{
	FireBulletData(const Vector& eyePos, Entity* entity) : src(eyePos), filter(entity)
	{
	}

	Vector          src;
	Trace           enter_trace;
	Vector          direction;
	TraceFilter     filter;
	float           trace_length;
	float           trace_length_remaining;
	float           current_damage;
	int             penetrate_count;
};


class AutoWall
{
public:
	float Damage(const Vector& point, Vector& wallbangVector, float minDamage)noexcept;
	float EntityDamage(Entity* entity, const Vector& point) noexcept;
	float EntityDamageFromVector(Entity* entity, const Vector& point, const Vector& pos) noexcept;
	bool CanHitFloatingPoint(const Vector& point, const Vector& source, Vector& wallbangPos)noexcept;
	bool CanWallbang(float& dmg, Vector& wallbangPos)noexcept;
	bool PenetrateWall(Entity* pBaseEntity, Vector& vecPoint, int weaponIndex, Vector& wallbangVector) noexcept;
	void UTIL_ClipTraceToPlayers(const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, TraceFilter* filter, Trace* tr)noexcept;
};


extern AutoWall* Autowall;