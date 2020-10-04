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
    };

    struct Record {
        float prevSimTime = 0.0f;
        int prevhealth = 0;
        int lastworkingshot = -1; 
        int missedshots = 0;
        bool wasTargeted = false;
        bool invalid = true;
        bool FiredUpon = false;
        bool autowall = false;
        float PreviousEyeAngle = 0.0f;
        float eyeAnglesOnUpdate = 0.0f;
        float PreviousDesyncAng = 0.0f;
        float originalLBY = 0.0f; 
        float velocity = -1.0f;
        float prevVelocity = -1.0f;
        float multiExpan = 2.0f;
        float lbyNextUpdate = 0.0f;
        bool lbyUpdated = false;
        //Entity* activeweapon;
        std::deque<shotdata> shots;
        bool wasUpdated = false;
    };

    extern Record invalid_record;
    //extern std::vector<Record> PlayerRecords(65);
    extern std::vector<Record> PlayerRecords;
    extern Entity* TargetedEntity;


    void BasicResolver(Entity* entity, int missed_shots);
    void UpdateTargeted(Entity* entity, bool targeted);
    
    void CalculateHits(GameEvent event);
    bool LBY_UPDATE(Entity* entity, Resolver::Record* record);
    void Update();

}