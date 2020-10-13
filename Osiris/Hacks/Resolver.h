#pragma once

#include "../SDK/Entity.h"
#include "../Config.h"
#include <deque>
#include <vector>


namespace Resolver {

    /* 
      
       TODO: On game event bullet_impact, trace a ray from user to point, and see if it intersects/touches a player. If so, resolver miss, otherwise
       spread miss.
       
       void MissedDueToSpread(Gameevent &event);
    */
    struct shotdata {
        matrix3x4 matrix[256];
        float simtime;
        Vector EyePosition;
        Vector TargetedPosition;
        Vector viewangles;
        int DamageDone;
        int shotCount;
        int hitbox;
    };

    struct LBYResolveBackup {
        bool Prev = false;
        float EyeAngle;
        Vector AbsAngle;
    };

    struct AngleSet {
        bool Set = false;
        Vector EyeAngles = { 0,0,0 };
        Vector ABSAngles = { 0,0,0 };
        float LBYAngle = 0.0f;
    };

    struct ResolveBackupData {
        float prevSimTime = 0.0f;
        bool wasUpdated = false;

        AngleSet ResolveAddition;
        AngleSet Original;
        AngleSet CurrentSet;
    };

    struct Record {
        int prevhealth =0;
        int lastworkingshot = -1;
        int missedshots = 0;
        bool wasTargeted = false;
        bool invalid = true;
        bool FiredUpon = false;
        bool autowall = false;
        float velocity = -1.0f;
        float prevVelocity = -1.0f;
        float multiExpan = 2.0f;
        float lbyNextUpdate = 0.0f;
        bool lbyUpdated = false;
        bool onshot = false;
        int missedshotsthisinteraction = 0;
        int totalshots = 0;
        bool noDesync = false;
        std::deque<shotdata> shots;
        LBYResolveBackup lbybackup;
        ResolveBackupData ResolveInfo;
        std::wstring PlayerName;
    };

    extern Record invalid_record;
    //extern std::vector<Record> PlayerRecords(65);
    extern std::vector<Record> PlayerRecords;
    extern Entity* TargetedEntity;


    //void BasicResolver(Entity* entity, int missed_shots);
    void AnimStateResolver(Entity* entity);
    void UpdateTargeted(Entity* entity, bool targeted);
    void Reset(FrameStage stage);
    void CalculateHits(GameEvent event);
    //bool LBY_UPDATE(Entity* entity, Resolver::Record* record);
    bool LBY_UPDATE(Entity* entity, Resolver::Record* record, bool UseAnim = false, int TicksToPredict = 0);
    void Update();

}