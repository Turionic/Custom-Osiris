#pragma once

//class Vector;
#include "SDK/Vector.h"

class CTeslaInfo
{
public:
	Vector			m_vPos;
	Vector			m_vAngles;
	int				m_nEntIndex;
	const char* m_pszSpriteName;
	float			m_flBeamWidth;
	int				m_nBeams;
	Vector			m_vColor;
	float			m_flTimeVisible;
	float			m_flRadius;
};

enum ParticleAttachment_t
{
	PATTACH_ABSORIGIN = 0,			// Create at absorigin, but don't follow
	PATTACH_ABSORIGIN_FOLLOW,		// Create at absorigin, and update to follow the entity
	PATTACH_CUSTOMORIGIN,			// Create at a custom origin, but don't follow
	PATTACH_POINT,					// Create on attachment point, but don't follow
	PATTACH_POINT_FOLLOW,			// Create on attachment point, and update to follow the entity

	PATTACH_WORLDORIGIN,			// Used for control points that don't attach to an entity

	PATTACH_ROOTBONE_FOLLOW,		// Create at the root bone of the entity, and update to follow

	MAX_PATTACH_TYPES,
};

struct te_tf_particle_effects_colors_t
{
	Vector m_vecColor1;
	Vector m_vecColor2;
};

struct te_tf_particle_effects_control_point_t
{
	ParticleAttachment_t m_eParticleAttachment;
	Vector m_vecOffset;
};

class CEffectData
{
public:
	Vector m_vOrigin;
	Vector m_vStart;
	Vector m_vNormal;
	Vector m_vAngles;
	int		m_fFlags;
	int		m_nEntIndex;
	float	m_flScale;
	float	m_flMagnitude;
	float	m_flRadius;
	int		m_nAttachmentIndex;
	short	m_nSurfaceProp;

	// Some TF2 specific things
	int		m_nMaterial;
	int		m_nDamageType;
	int		m_nHitBox;

	unsigned char	m_nColor;

	// Color customizability
	bool							m_bCustomColors;
	te_tf_particle_effects_colors_t	m_CustomColors;

	bool									m_bControlPoint1;
	te_tf_particle_effects_control_point_t	m_ControlPoint1;

	// Don't mess with stuff below here. DispatchEffect handles all of this.
public:
	CEffectData()
	{
		m_vOrigin;
		m_vStart;
		m_vNormal;
		m_vAngles;

		m_fFlags = 0;
		m_nEntIndex = 0;
		m_flScale = 1.f;
		m_nAttachmentIndex = 0;
		m_nSurfaceProp = 0;

		m_flMagnitude = 0.0f;
		m_flRadius = 0.0f;

		m_nMaterial = 0;
		m_nDamageType = 0;
		m_nHitBox = 0;

		m_nColor = 0;

		m_bCustomColors = false;
		m_CustomColors.m_vecColor1 = { 1.f, 1.f, 1.f };
		m_CustomColors.m_vecColor2 = { 1.f, 1.f, 1.f };

		m_bControlPoint1 = false;
		m_ControlPoint1.m_eParticleAttachment = PATTACH_ABSORIGIN;
		m_ControlPoint1.m_vecOffset;
	}

	int GetEffectNameIndex() { return m_iEffectName; }

private:
	int m_iEffectName;	// Entry in the EffectDispatch network string table. The is automatically handled by DispatchEffect().
};


#include "Memory.h"
class IEffects
{
public:
	virtual ~IEffects() {};

	virtual void Beam(const Vector& Start, const Vector& End, int nModelIndex,
		int nHaloIndex, unsigned char frameStart, unsigned char frameRate,
		float flLife, unsigned char width, unsigned char endWidth, unsigned char fadeLength,
		unsigned char noise, unsigned char red, unsigned char green,
		unsigned char blue, unsigned char brightness, unsigned char speed) = 0;

	//-----------------------------------------------------------------------------
	// Purpose: Emits smoke sprites.
	// Input  : origin - Where to emit the sprites.
	//			scale - Sprite scale * 10.
	//			framerate - Framerate at which to animate the smoke sprites.
	//-----------------------------------------------------------------------------
	virtual void Smoke(const Vector& origin, int modelIndex, float scale, float framerate) = 0;

	virtual void Sparks(const Vector& position, int nMagnitude = 1, int nTrailLength = 1, const Vector* pvecDir = NULL) = 0;

	virtual void Dust(const Vector& pos, const Vector& dir, float size, float speed) = 0;

	virtual void MuzzleFlash(const Vector& vecOrigin, const Vector& vecAngles, float flScale, int iType) = 0;

	// like ricochet, but no sound
	virtual void MetalSparks(const Vector& position, const Vector& direction) = 0;

	virtual void EnergySplash(const Vector& position, const Vector& direction, bool bExplosive = false) = 0;

	virtual void Ricochet(const Vector& position, const Vector& direction) = 0;

	// FIXME: Should these methods remain in this interface? Or go in some 
	// other client-server neutral interface?
	virtual float Time() = 0;
	virtual bool IsServer() = 0;

	// Used by the playback system to suppress sounds
	virtual void SuppressEffectsSounds(bool bSuppress) = 0;

	void DispatchEffect(const CEffectData& data)
	{
		using type_fn = bool(__cdecl*)(const CEffectData&);
		static auto fn = reinterpret_cast<type_fn>(memory->findPattern_ex(L"client", "\x55\x8B\xEC\x8B\x4D\x08\xF3\x0F\x10\x51?\x8D"));

		fn(data);
	}
};

namespace EffectFunction {
	
	using FX_TeslaType = void(__thiscall*)(CTeslaInfo&);
	const auto TeslaFunc = reinterpret_cast<FX_TeslaType>(memory->findPattern_ex(L"client", "\x55\x8B\xEC\x81\xEC????\x56\x57\x8B\xF9\x8B\x47\x18"));
}


#include "SDK/VirtualMethod.h"
#include <functional>

struct ColorRGBExp32
{
	std::byte r, g, b;
	signed char exponent;
};

struct dlight_t
{
	int		flags;
	Vector	origin;
	float	radius;
	ColorRGBExp32 color;
	float	die;
	float	decay;
	float	minlight;
	int		key;
	int		style;
	Vector	m_Direction;
	float	m_InnerAngle;
	float	m_OuterAngle;
	float GetRadius() const
	{
		return radius;
	}
	float GetRadiusSquared() const
	{
		return radius * radius;
	}
	float IsRadiusGreaterThanZero() const
	{
		return radius > 0.0f;
	}
};

class IVEffects
{
public:
	VIRTUAL_METHOD(dlight_t*, CL_AllocDlight, 4, (int key), (this, key));
	VIRTUAL_METHOD(dlight_t*, CL_AllocElight, 5, (int key), (this, key));
	VIRTUAL_METHOD(dlight_t*, GetElightByKey, 8, (int key), (this, key));
};

/*

.rdata:00B309A0                 dd offset ??_R4IEffectsList@@6B@ ; const IEffectsList::`RTTI Complete Object Locator'
.rdata:00B309A4 ; const IEffectsList::`vftable'
.rdata:00B309A4 ??_7IEffectsList@@6B@ dd offset sub_27C590
.rdata:00B309A4                                         ; DATA XREF: sub_27C590+A↑o
.rdata:00B309A4                                         ; sub_27C610+A↑o ...
.rdata:00B309A8                 dd offset __purecall
.rdata:00B309AC                 dd offset __purecall
.rdata:00B309B0                 dd offset __purecall
.rdata:00B309B4 aEffectsBluespa db 'effects/bluespark',0
.rdata:00B309B4                                         ; DATA XREF: sub_27C7B0+39↑o
.rdata:00B309B4                                         ; sub_740580+C0↑o
.rdata:00B309C6                 align 4
.rdata:00B309C8 aLine           db 'Line',0             ; DATA XREF: sub_27C800+1C↑o
.rdata:00B309C8                                         ; sub_6ABDB0+3B↑o ...
.rdata:00B309CD                 align 10h
.rdata:00B309D0 aPrecacheeffect_1 db 'PrecacheEffectsTest',0
.rdata:00B309D0                                         ; DATA XREF: .data:00D4A3D8↓o
.rdata:00B309E4 aEffectsGunship db 'effects/gunshiptracer',0
.rdata:00B309E4                                         ; DATA XREF: sub_27C7B0+28↑o
.rdata:00B309FA                 align 4
.rdata:00B309FC aCbulletwhiztim db 'CBulletWhizTimer',0 ; DATA XREF: sub_347B0↑o
.rdata:00B30A0D                 align 10h
.rdata:00B30A10 a72             db '72',0               ; DATA XREF: sub_347E0+1F↑o
.rdata:00B30A10                                         ; sub_489070+36↑o
.rdata:00B30A13                 align 4
.rdata:00B30A14 aFxPlayertracer db 'FX_PlayerTracer',0  ; DATA XREF: sub_27C8D0+28↑o
.rdata:00B30A24 aBulletsGunship db 'Bullets.GunshipNearmiss',0
.rdata:00B30A24                                         ; DATA XREF: sub_27CB50:loc_27CCB6↑o
.rdata:00B30A3C aBulletsStrider db 'Bullets.StriderNearmiss',0
.rdata:00B30A3C                                         ; DATA XREF: sub_27CB50:loc_27CCBD↑o
.rdata:00B30A54 aClTracerWhizDi db 'cl_tracer_whiz_distance',0
.rdata:00B30A54                                         ; DATA XREF: sub_347E0+24↑o
.rdata:00B30A6C aBulletsDefault db 'Bullets.DefaultNearmiss',0
.rdata:00B30A6C                                         ; DATA XREF: sub_27CB50+7B↑o
.rdata:00B30A6C                                         ; sub_2A7F90+28↑o
.rdata:00B30A84 aUnderwaterBull db 'Underwater.BulletImpact',0
.rdata:00B30A84                                         ; DATA XREF: sub_27CB50+17C↑o
.rdata:00B30A9C aFxTracer       db 'FX_Tracer',0        ; DATA XREF: sub_27CED0+28↑o
.rdata:00B30AA6                 align 4

*/

/*

.rdata:00B392D4 aPrecacheeffect_4 db 'PrecacheEffectSplash',0
.rdata:00B392D4                                         ; DATA XREF: .data:00D4B874↓o
.rdata:00B392E9                 align 4
.rdata:00B392EC aClShowSplashes db 'cl_show_splashes',0 ; DATA XREF: sub_37C10+24↑o
.rdata:00B392FD                 align 10h
.rdata:00B39300 aFxWaterripple  db 'FX_WaterRipple',0   ; DATA XREF: sub_2A88D0+25↑o
.rdata:00B39300                                         ; sub_2A9180+10↑o ...
.rdata:00B3930F                 align 10h
.rdata:00B39310 aEffectsSplash4 db 'effects/splash4',0  ; DATA XREF: sub_2A8790+39↑o
.rdata:00B39320 aEffectsSlime1  db 'effects/slime1',0   ; DATA XREF: sub_2A8790+4A↑o
.rdata:00B3932F                 align 10h
.rdata:00B39330 aWaterSplash01  db 'water_splash_01',0  ; DATA XREF: sub_2A8A10+AE↑o
.rdata:00B39330                                         ; sub_2A8BF0+17↑o
.rdata:00B39340 aWaterSplash02  db 'water_splash_02',0  ; DATA XREF: sub_2A8A10+DC↑o
.rdata:00B39340                                         ; sub_2A8BF0+28↑o
.rdata:00B39350 aWaterSplash02S db 'water_splash_02_surface2',0
.rdata:00B39350                                         ; DATA XREF: sub_2A88D0+F0↑o
.rdata:00B39350                                         ; sub_2A89F0+10↑o
.rdata:00B39369                 align 4
.rdata:00B3936C aFxGunshotsplas db 'FX_GunshotSplash',0 ; DATA XREF: sub_2A8A10+2D↑o
.rdata:00B3936C                                         ; sub_2A8C50+2B↑o ...
.rdata:00B3937D                 align 10h
.rdata:00B39380 aSlimeSplash01  db 'slime_splash_01',0  ; DATA XREF: sub_2A8CD0+BD↑o
.rdata:00B39380                                         ; sub_2A8EB0+17↑o
.rdata:00B39390 aSlimeSplash02  db 'slime_splash_02',0  ; DATA XREF: sub_2A8CD0+EC↑o
.rdata:00B39390                                         ; sub_2A8EB0+28↑o
.rdata:00B393A0 aWaterSplash03  db 'water_splash_03',0  ; DATA XREF: sub_2A8A10+E3↑o
.rdata:00B393A0                                         ; sub_2A8BF0+39↑o
.rdata:00B393B0 aFxGunshotslime db 'FX_GunshotSlimeSplash',0
.rdata:00B393B0                                         ; DATA XREF: sub_2A8CD0+49↑o
.rdata:00B393B0                                         ; sub_2A8FB0+13↑o ...
.rdata:00B393C6                 align 4
.rdata:00B393C8 aWatersplashqui db 'watersplashquiet',0 ; DATA XREF: .data:off_D4B924↓o
.rdata:00B393C8                                         ; .data:00D4B938↓o
.rdata:00B393D9                 align 4
.rdata:00B393DC aWaterripple    db 'waterripple',0      ; DATA XREF: sub_3BC9D0+39D↑o
.rdata:00B393DC                                         ; .data:off_D4B95C↓o ...
.rdata:00B393E8 aSlimeSplash03  db 'slime_splash_03',0  ; DATA XREF: sub_2A8CD0+F3↑o
.rdata:00B393E8                                         ; sub_2A8EB0+39↑o
.rdata:00B393F8 aWatersplash    db 'watersplash',0      ; DATA XREF: sub_3BC9D0+27E↑o
.rdata:00B393F8                                         ; .data:off_D4B908↓o ...

*/