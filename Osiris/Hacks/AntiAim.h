#pragma once
#include "../SDK/Vector.h"
#include "../SDK/Entity.h"
#include "../SDK/NetworkChannel.h"
struct UserCmd;
struct Vector;

namespace AntiAim {
    //void GetMovementFix(unsigned int state, float oForwardMove, float oSideMove, UserCmd *cmd);
    void CorrectMovement(float OldAngleY, UserCmd* pCmd, float fOldForward, float fOldSidemove);
    bool LBY_UPDATE(Entity* entity, int TicksToPredict, bool UseAnim);
    float DEG2RAD(float Degress);
    void run(UserCmd*, const Vector&, const Vector&, bool&) noexcept;
    void fakeWalk(UserCmd* cmd, bool& sendPacket, const Vector& currentViewAngles) noexcept;
    float ToWall() noexcept;

    struct playerAAInfo {
        float lastlbyval = 0.0f;
        float lbyNextUpdate = 0.0f;
        bool b_Side = false;
        Vector real = { 0,0,0 };
        Vector fake = { 0,0,0 };
        Vector lby = { 0,0,0 };
        float PreBreakAngle = 0.0f;
        float PreBreakAngleFake = 0.0f;
        bool netset = false;
        NetworkChannel netchan;
        int lastOutgoing = 0;
    };

    extern bool lbyNextUpdatedPrevtick;
    extern bool lbyNextUpdated;
    extern playerAAInfo LocalPlayerAA;

}


