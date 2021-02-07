#pragma once

#include "VirtualMethod.h"
#include "Entity.h"

class Entity;
//class MoveData;

class MoveData
{
	//public:
	//    char pad_0x00[183];
public:
	bool            m_bFirstRunOfFunctions : 1;
	bool            m_bGameCodeMovedPlayer : 1;
	bool            m_bNoAirControl : 1;

	Entity* m_nPlayerHandle;    // edict index on server, client entity handle on client EntityHandle_t

	int                m_nImpulseCommand;    // Impulse command issued.
	Vector            m_vecViewAngles;    // Command view angles (local space)
	Vector            m_vecAbsViewAngles;    // Command view angles (world space)
	int                m_nButtons;            // Attack buttons.
	int                m_nOldButtons;        // From host_client->oldbuttons;
	float            m_flForwardMove;
	float            m_flSideMove;
	float            m_flUpMove;

	float            _m_flMaxSpeed;
	float            m_flClientMaxSpeed;

	// Variables from the player edict (sv_player) or entvars on the client.
	// These are copied in here before calling and copied out after calling.
	Vector            m_vecVelocity_;        // edict::velocity        // Current movement direction.
	Vector            m_vecOldVelocity;
	float            somefloat;
	Vector            m_vecAngles;        // edict::angles
	Vector            m_vecOldAngles;

	// Output only
	float            m_outStepHeight;    // how much you climbed this move
	Vector            m_outWishVel;        // This is where you tried 
	Vector            m_outJumpVel;        // This is your jump velocity

										   // Movement constraints    (radius 0 means no constraint)
	Vector            m_vecConstraintCenter;
	float            m_flConstraintRadius;
	float            m_flConstraintWidth;
	float            m_flConstraintSpeedFactor;
	bool            m_bConstraintPastRadius;        ///< If no, do no constraining past Radius.  If yes, cap them to SpeedFactor past radius

	//void            SetAbsOrigin(const Vector& vec);
	//const Vector& GetAbsOrigin() const;

};

// https://github.com/click4dylan/CSGO_GameMovement_Reversed/blob/master/IGameMovement.h
class GameMovement {
public:




	VIRTUAL_METHOD(void, processMovement, 1, (Entity* localPlayer, MoveData* moveData), (this, localPlayer, moveData))
	VIRTUAL_METHOD(void, Reset, 2, (), (this))
    VIRTUAL_METHOD(void, StartTrackPredictionErrors, 3, (Entity* entity), (this, localPlayer))
	VIRTUAL_METHOD(void, FinishTrackPredictionErrors, 4, (Entity* entity), (this, localPlayer))
	VIRTUAL_METHOD(Vector&, GetPlayerMins, 6, (bool ducked), (this, false))
	VIRTUAL_METHOD(Vector&, GetPlayerMaxs, 7, (bool ducked), (this, false))
	VIRTUAL_METHOD(Vector&, GetPlayerViewOffset, 8, (bool ducked), (this, false))

	/*
	virtual void Reset();// 2
	virtual void StartTrackPredictionErrors(CBasePlayer* pPlayer); // 3
	virtual void FinishTrackPredictionErrors(CBasePlayer* pPlayer); // 4
	virtual void DiffPrint(char const* fmt, ...); // 5
	virtual const Vector& GetPlayerMins(bool ducked) const; // 6
	virtual const Vector& GetPlayerMaxs(bool ducked) const; // 7
	virtual const Vector& GetPlayerViewOffset(bool ducked) const; // 8
	virtual bool IsMovingPlayerStuck() const; // 9
	virtual CBasePlayer* GetMovingPlayer() const; // 10
	virtual void UnblockPusher(CBasePlayer* pPlayer, CBaseEntity* pPusher); // 11
	virtual void SetupMovementBounds(CMoveData* moveData); // 12
	virtual Vector GetPlayerMins(); // 13
	virtual Vector GetPlayerMaxs(); // 14
	virtual void TracePlayerBBox(const Vector& rayStart, const Vector& rayEnd, int fMask, int collisionGroup, trace_t& tracePtr); // 15
	virtual unsigned int PlayerSolidMask(bool brushOnly = false, CBasePlayer* testPlayer = nullptr); // 16
	virtual void PlayerMove(); // 17
	virtual float CalcRoll(const QAngle& angles, const Vector& velocity, float rollangle, float rollspeed); // 18
	virtual void DecayViewPunchAngle(); // 19
	virtual void CheckWaterJump(); // 20
	virtual void WaterMove(); // 21
	virtual void SlimeMove(); //22
	virtual void WaterJump(); // 23
	virtual void Friction(); // 24
	virtual void AirAccelerate(Vector& wishdir, float wishspeed, float accel); // 25
	virtual void AirMove(); // 26
	virtual bool CanAccelerate(); // 27
	virtual void Accelerate(Vector& wishdir, float wishspeed, float accel); // 28
	virtual void WalkMove(); // 29
	virtual void StayOnGround(); // 30
	virtual void FullWalkMove(); // 31
	virtual void OnJump(float stamina); // 32
	virtual void nullsub3(); // 33
	virtual void OnLand(float flFallVelocity); // 34
	bool		CheckInterval(IntervalType_t type);
	virtual int GetCheckInterval(IntervalType_t type); // 35
	virtual void StartGravity(); // 36
	virtual void FinishGravity(); // 37
	virtual void AddGravity(); // 38
	virtual bool CheckJumpButton(); // 39
	virtual void FullTossMove(); // 40
	virtual void FullLadderMove(); // 41
	virtual int TryPlayerMove(Vector* pFirstDest = NULL, trace_t* pFirstTrace = NULL); // 42
	virtual bool LadderMove(); // 43
	virtual bool OnLadder(trace_t& pm); // 44
	virtual float LadderDistance(); // 45
	virtual unsigned int LadderMask(); // 46
	virtual float ClimbSpeed(); // 47
	virtual float LadderLateralMultiplier(); // 48
	void   CheckVelocity(void);
	virtual int ClipVelocity(Vector& in, Vector& normal, Vector& out, float overbounce); // 49
	virtual bool CheckWater(); // 50
	virtual void GetWaterCheckPosition(int waterLevel, Vector* pos); // 51
	virtual void CategorizePosition(); // 52
	virtual void CheckParameters(); // 53
	virtual void ReduceTimers(); // 54
	virtual void CheckFalling(); // 55
	virtual void PlayerRoughLandingEffects(float fVol); // 56
	virtual void Duck();// 57
	virtual void HandleDuckingSpeedCrop(); //58
	virtual void FinishUnduck(); //59
	virtual void FinishDuck(); // 60
	virtual bool CanUnduck(); // 61
	virtual void UpdateDuckJumpEyeOffset(); // 62
	virtual bool CanUnduckJump(trace_t& tr); // 63
	virtual void StartUnduckJump(); // 64
	virtual void FinishUnduckJump(trace_t& tr); // 65
	virtual void SetDuckedEyeOffset(float duckFraction); // 66
	virtual void FixPlayerCrouchStuck(bool upward); // 67
	virtual void CategorizeGroundSurface(trace_t& tr); // 68
	virtual bool InWater(); // 69
	virtual CBaseHandle TestPlayerPosition(const Vector& pos, int collisionGroup, trace_t& pm); // 70
	virtual void SetGroundEntity(trace_t* pm); // 71
	virtual void StepMove(Vector& vecDestination, trace_t& trace); // 72
	virtual ITraceFilter* LockTraceFilter(int collisionGroup); // 73
	virtual void UnlockTraceFilter(ITraceFilter*& filter); // 74
	virtual bool GameHasLadders(); // 75
	void PerformFlyCollisionResolution(trace_t& pm, Vector& move);
	void PushEntity(Vector& push, trace_t* pTrace);
	void ResetGetWaterContentsForPointCache();
	int GetWaterContentsForPointCached(const Vector& point, int slot);
	float ComputeConstraintSpeedFactor(void);
	int CheckStuck(void);
	void FullNoClipMove(float factor, float maxacceleration);
	void FullObserverMove(void);
	*/
	enum
	{
		// eyes, waist, feet points (since they are all deterministic
		MAX_PC_CACHE_SLOTS = 3,
	};

	Entity* player; //4
	MoveData* mv; //8
	int m_nOldWaterLevel; //12
	float m_flWaterEntryTime; //16
	int m_nOnLadder; //20
	Vector m_vecForward; //24
	Vector m_vecRight; //36
	Vector m_vecUp; //48
	int m_CachedGetPointContents[64][MAX_PC_CACHE_SLOTS];
	Vector m_CachedGetPointContentsPoint[64][MAX_PC_CACHE_SLOTS];
	BOOL m_bSpeedCropped; //3132
	bool m_bProcessingMovement; //3136
	bool m_bInStuckTest; //3137
	float			m_flStuckCheckTime[64 + 1][2]; //3138
	//ITraceListData* m_pTraceListData; //3660
	//int m_nTraceCount; //3664
};


