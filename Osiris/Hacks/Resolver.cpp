#include "Resolver.h"

#include "../Config.h"
#include "../Interfaces.h"
#include "../Memory.h"
#include "../Netvars.h"
#include "Misc.h"
#include "../SDK/ConVar.h"
#include "../SDK/Surface.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/NetworkChannel.h"
#include "../SDK/WeaponData.h"
#include "EnginePrediction.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/Entity.h"
#include "../SDK/UserCmd.h"
#include "../SDK/GameEvent.h"
#include "../SDK/FrameStage.h"
#include "../SDK/Client.h"
#include "../SDK/ItemSchema.h"
#include "../SDK/WeaponSystem.h"
#include "../SDK/WeaponData.h"
#include "../SDK/Vector.h"
#include "../GUI.h"
#include "../Helpers.h"
#include "../SDK/ModelInfo.h"
#include "Backtrack.h"
#include "../SDK/AnimState.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/FrameStage.h"
#include <deque>


#include "../Debug.h"
#include <mutex>
#include <numeric>
#include <sstream>
#include <codecvt>
#include <locale>
#include <string>
#include <iostream>
#include <cstddef>

const int MAX_RECORDS = 128;

std::vector<Resolver::Record> Resolver::PlayerRecords(65);
Resolver::Record Resolver::invalid_record;
Entity* Resolver::TargetedEntity;

//#include "../Debug.h"
static void ResolverDebug(std::wstring str) {
    if (config->debug.resolverDebug) {
        Debug::LogItem item;
        item.PrintToScreen = false;
        item.text.push_back(str);
        Debug::LOG_OUT.push_front(item);
    }

}

void Resolver::CalculateHits(GameEvent event) {
    /* Calculate Spread Misses
       Calculate Hitbox hit
       Calculate Damage Done
       Whether to inc missed shots
       etc 
     */



}

void Resolver::Reset(FrameStage stage) {


    for (int i = 0; i <= interfaces->engine->getMaxClients(); i++) {
        Entity* entity = interfaces->entityList->getEntity(i);
        if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive()
            || !entity->isOtherEnemy(localPlayer.get()))
            continue;

        Record* record = &PlayerRecords.at(entity->index());

        if (!record || record->invalid || !record->ResolveInfo.Original.Set)
            return;

        AnimState* Animstate = entity->getAnimstate();
        if (!Animstate)
            return;

        
        record->FiredUpon = false;

        Animstate->m_flGoalFeetYaw = record->ResolveInfo.Original.LBYAngle; /* Restore */
        entity->setAbsAngle(record->ResolveInfo.Original.ABSAngles);
        entity->eyeAngles() = record->ResolveInfo.Original.EyeAngles;
        entity->UpdateState(Animstate, entity->eyeAngles());


    }


}



/*
bool Resolver::LBY_UPDATE(Entity* entity, Resolver::Record* record) {

    bool returnval = false;
    float servertime = memory->globalVars->serverTime();

    if (record->velocity > 0.1f) { //LBY updates on any velocity
        record->lbyNextUpdate = 0.22f + servertime;
        return false;
    }

    if (record->lbyNextUpdate < servertime) {
        record->lbyNextUpdate = servertime + 1.1f;
        return true;
    }
    else {
        return false;
    }    
}
*/


bool Resolver::LBY_UPDATE(Entity* entity, Resolver::Record* record, bool UseAnim, int TicksToPredict){
    float servertime = memory->globalVars->serverTime();
    float Velocity;

    if (!(TicksToPredict == 0)) { servertime += memory->globalVars->intervalPerTick * TicksToPredict; }

    // if(!TicksToPredict){} <-- this is a shit way to determine whether to set lbyNextUpdate but oh well
    if (!UseAnim) {
        Velocity = entity->getVelocity().length2D();
    }
    else {
        AnimState* as_EntityAnimState = entity->getAnimstate();
        if (as_EntityAnimState) {
            //return false;
            try {
                //Velocity = as_EntityAnimState->speed_2d;
                Velocity = entity->getVelocity().length2D();
                /* So For some reason CS:GO Likes to give me pointers to random addresses when calling
                   getAnimstate. So to combat this, we just wont use it until I can find a fix.*/

            }
            catch (std::exception& e) { /* This likes to be thrown.. often. FIX THIS!*/
                ResolverDebug(std::wstring{ L"Error [Resolver.cpp:107]: as_EntityAnimState is an Invalid Pointer" });
                Velocity = entity->getVelocity().length2D();
            }
        }
        else {
            Velocity = entity->getVelocity().length2D();
        }
    }

    if (Velocity > 0.1f) { //LBY updates on any velocity
        if (TicksToPredict == 0) { record->lbyNextUpdate = 0.22f + servertime;}
        return false; // FALSE, as I dont want to run LBY breaking code on movement!, however it does update!!!
    }

    if ( record->lbyNextUpdate >= servertime) {
        return false;
    }
    else if ( record->lbyNextUpdate < servertime) { // LBY ipdates on no velocity, .22s after last velocity, 1.1s after previous no-velocity
        if (TicksToPredict == 0) { record->lbyNextUpdate = servertime + 1.1f;}
        return true;
    }

    return false;
}


void Resolver::Update() {

    for (int i = 0; i < PlayerRecords.size(); i++) {

        if (!config->debug.animstatedebug.resolver.enabled)
            return;

        if (!localPlayer || !localPlayer->isAlive()) {
            //return;
            auto record = &PlayerRecords.at(i);
            if (!record)
                continue;
            record->invalid = true;
            record->shots.clear();
            record->totalshots = 0;
            continue;
        }

        auto record = &PlayerRecords.at(i);

        if (!record)
            continue;

        record->move = false;
        record->noDesync = false;

        auto entity = interfaces->entityList->getEntity(i);

        if (!entity || !entity->isPlayer() || entity->isDormant() || !entity->isAlive() ||!entity->isOtherEnemy(localPlayer.get()) || entity == localPlayer.get()) { //  ((entity != localPlayer.get()) && 
            if (record->FiredUpon || record->wasTargeted) {
                record->lastworkingshot = record->missedshots;
            }
            record->missedshotsthisinteraction = 0;
            record->totalshots = 0;
            record->invalid = true;
            record->ResolveInfo.Original.Set = false;
            //record->shots.clear();
            continue;
        }

        record = &PlayerRecords.at(entity->index());

        if (!record)
            continue;

        record->prevVelocity = record->velocity;
        record->velocity = entity->getVelocity().length2D();
        if (record->invalid == true) {
            record->PlayerName = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(entity->getPlayerName(true));
            record->ResolveInfo.CurrentSet.Set = false;
            record->ResolveInfo.Original.Set = true;
            record->ResolveInfo.Original.ABSAngles = entity->getAbsAngle();
            record->ResolveInfo.Original.EyeAngles = entity->eyeAngles();

            record->missedshotsthisinteraction = 0;
            record->totalshots = 0;
            if (config->debug.animstatedebug.resolver.missedshotsreset) { 
                record->missedshots = config->debug.animstatedebug.resolver.missedoffset; 
            }
            record->prevhealth = entity->health();
            record->wasTargeted = false;
            record->FiredUpon = false;
            record->multiExpan = 2.0f;
        }
        
        record->lbyUpdated = LBY_UPDATE(entity, record, true);
        //record->activeweapon = entity->getActiveWeapon();
        record->onshot = false;
        auto Animstate = entity->getAnimstate();
        if (Animstate && record->wasTargeted) {
            record->ResolveInfo.Original.LBYAngle = Animstate->m_flGoalFeetYaw; /* Save What We Get From The Server*/
            auto AnimLayerOne = entity->getAnimationLayer(1);
            if (AnimLayerOne) {
                auto currAct = entity->getSequenceActivity(AnimLayerOne->sequence);
                bool IN_FIRE_ACT = (currAct == ACT_CSGO_FIRE_PRIMARY) || (currAct == ACT_CSGO_FIRE_PRIMARY_OPT_1) || (currAct == ACT_CSGO_FIRE_PRIMARY_OPT_2);
                if (IN_FIRE_ACT && (AnimLayerOne->weight > (0.1f)) && (AnimLayerOne->cycle < .9f)) {
                    record->onshot = true;
                }

            }
        }

        if (!record->shots.empty()) {
            
            if ((entity->simulationTime()) > (record->shots.back().simtime + 4.0f)) {
                record->shots.pop_back();
            }
            
            if (record->shots.size() > 5) {
                record->shots.erase(record->shots.begin() + 5, record->shots.end());
            }
        }

        if (!record->FiredUpon) {
            record->invalid = false;
            continue;
        }
        else if (((record->prevhealth == entity->health()) && !record->invalid) || (config->debug.animstatedebug.resolver.goforkill) || ((record->shots.size() > 1) && record->shots.back().hitbox == 0)){
            record->lastworkingshot = -1;
            record->missedshots =  ++(record->missedshots); // (record->missedshots >= 8 ? config->debug.animstatedebug.resolver.missedoffset )
            record->ResolveInfo.wasUpdated = false;
            record->missedshotsthisinteraction++;
            record->totalshots++;
            Debug::LogItem item;
            item.text.push_back(std::wstring{ L"Missed Shot On " + record->PlayerName + L" Fuck If I know why, GL resolving next time" });
            Debug::LOG_OUT.push_front(item);
        }
        else if (!record->shots.empty()) {
            record->shots.back().DamageDone = record->prevhealth - entity->health(); 
            record->totalshots++;
            if (config->debug.animstatedebug.resolver.missedshotsreset) {
                record->missedshots = 0;
            }
        }
        else {
            record->totalshots++;
            if (config->debug.animstatedebug.resolver.missedshotsreset) {
                record->missedshots = 0;
            }
        }
        
        record->prevhealth = entity->health();

        record->invalid = false;

        if (record->wasTargeted) {
            TargetedEntity = entity;
        }

    }
}

/*

    struct shotdata {
        matrix3x4 matrix[256];
        float simtime;
        Vector EyePosition;
        Vector TargetedPosition;
        Vector viewangles;
        int DamageDone;
        int shotCount;
    };

    struct LBYResolveBackup {
        bool Prev = false;
        float EyeAngle;
        Vector AbsAngle;
    };

    struct ResolveBackupData{
        float PreviousEyeAngle = 0.0f;
        float eyeAnglesOnUpdate = 0.0f;
        float PreviousDesyncAng = 0.0f;
        float originalLBY = 0.0f;
    }

    struct Record {
        float prevSimTime = 0.0f;
        int prevhealth = 0;
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
        int missedshotsthisinteraction = 0;
        int totalshots = 0;
        //Entity* activeweapon;
        std::deque<shotdata> shots;
        bool wasUpdated = false;
        LBYResolveBackup lbybackup;
        ResolveBackupData ResolveInfo;
    };



*/





void Resolver::AnimStateResolver(Entity* entity) {

    // __ MAKE INTO FUNCTION __
    config->debug.indicators.resolver = false;
    if (!localPlayer || !localPlayer->isAlive() || !entity || !entity->isPlayer() || entity->isDormant() || !entity->isAlive() || !entity->isOtherEnemy(localPlayer.get()))
        return;

    if (PlayerRecords.empty())
        return;

    std::wstring name = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(entity->getPlayerName(true));
    auto record = &PlayerRecords.at(entity->index());

    if (!record || record->invalid) {
        //ResolverDebug(std::wstring{ L"[Resolver.cpp:321] Exiting, No record/invalid record" });
        record->FiredUpon = false;
        return;
    }


    if (record->onshot) {
        ResolverDebug(std::wstring{ L"[Resolver.cpp:314] Exiting, player onshot [" + name + L"] -> " + std::to_wstring(memory->globalVars->currenttime) });
        record->FiredUpon = false;
        return;
    }

    auto Animstate = entity->getAnimstate();

    if (!Animstate) {
        ResolverDebug(std::wstring{L"[Resolver.cpp:321] Exiting, No anim state [" + name + L"] -> " + std::to_wstring(memory->globalVars->currenttime) });
        record->FiredUpon = false;
        return;
    }

    auto AnimLayerThree = entity->getAnimationLayer(3);
    if (!AnimLayerThree) {
        ResolverDebug(std::wstring{ L"[Resolver.cpp:321] Exiting, No AnimLayerThree was set [" + name + L"] -> " + std::to_wstring(memory->globalVars->currenttime) });
        record->FiredUpon = false;
        return;
    }

    auto currAct = entity->getSequenceActivity(AnimLayerThree->sequence);
    bool IN_LBY_ACT = (currAct == ACT_CSGO_IDLE_TURN_BALANCEADJUST);

    float Velocity = entity->velocity().length2D();

    bool hasDesyncHadTimeToLBY = false;
    bool hasDesyncHadTimeToDesync = false;
    bool PresumedNoDesync = false;
    bool move = true;
    //bool btFLAG = false;
    float DesyncFrac = 1.0f;
    if ((Velocity < 140) && (Animstate->m_flTimeSinceStartedMoving <= 1.2f)) {
        if (Animstate->m_flTimeSinceStartedMoving <= 1.2f) {

            if (Animstate->m_flTimeSinceStoppedMoving >= .22f) {
                hasDesyncHadTimeToLBY = true;
                if (Animstate->m_flTimeSinceStoppedMoving >= .8f) {
                    hasDesyncHadTimeToDesync = true;
                }
            }
        } else if (Animstate->m_flTimeSinceStartedMoving > 0) {
            if (Velocity > 100) {
                DesyncFrac = 1.1f - (Animstate->m_flTimeSinceStartedMoving / 1.1f);
                move = true;
                hasDesyncHadTimeToDesync = true;
            } 
        }
    }
    else{
        PresumedNoDesync = true;
    }

    record->noDesync = PresumedNoDesync;
    record->move = move;


    if (PresumedNoDesync || move) {
        std::deque<Backtrack::Record>* bt_record = &(Backtrack::records[entity->index()]);
        if (bt_record && !bt_record->empty()) {
            bt_record->front().move = record->move;
            bt_record->front().noDesync = record->noDesync;
        }
    }


    if ((record->ResolveInfo.prevSimTime != entity->simulationTime()) && (entity->eyeAngles().y != record->ResolveInfo.Original.EyeAngles.y)) { /* On Character Update */
        record->ResolveInfo.prevSimTime = entity->simulationTime();
        entity->UpdateState(Animstate, entity->eyeAngles());

        if (!record->ResolveInfo.wasUpdated && !record->lbybackup.Prev) { /* If we didn't update it, we set it up. This is to prevent spinning characters*/
            record->ResolveInfo.Original.LBYAngle = Animstate->m_flGoalFeetYaw; /* Save What We Get From The Server*/
            record->ResolveInfo.Original.ABSAngles = entity->getAbsAngle();
            record->ResolveInfo.Original.EyeAngles = entity->eyeAngles();
            record->ResolveInfo.Original.Set = true;
            
            if (!PresumedNoDesync) {
                Animstate->m_flGoalFeetYaw += record->ResolveInfo.ResolveAddition.LBYAngle; /* Add on the Additions */
                entity->getAbsAngle() += record->ResolveInfo.ResolveAddition.ABSAngles;
                entity->eyeAngles() += record->ResolveInfo.ResolveAddition.EyeAngles;
            }

            record->ResolveInfo.CurrentSet.Set = true;
            record->ResolveInfo.CurrentSet.LBYAngle = Animstate->m_flGoalFeetYaw; /* Set Current Angles */
            record->ResolveInfo.CurrentSet.ABSAngles = entity->getAbsAngle();
            record->ResolveInfo.CurrentSet.EyeAngles = entity->eyeAngles();
            record->ResolveInfo.wasUpdated = false;

        }
        else if (record->lbybackup.Prev) {
            if (record->ResolveInfo.CurrentSet.Set) { /* Restore After LBY Tick*/
                if (hasDesyncHadTimeToLBY || hasDesyncHadTimeToDesync) {
                    Animstate->m_flGoalFeetYaw = record->ResolveInfo.CurrentSet.LBYAngle; /* Restore */
                    entity->getAbsAngle() = record->ResolveInfo.CurrentSet.ABSAngles;
                    entity->eyeAngles() = record->ResolveInfo.CurrentSet.EyeAngles;
                }
                entity->UpdateState(Animstate, entity->eyeAngles());
            }
            record->lbybackup.Prev = false;     
        }
        else {
            record->ResolveInfo.Original.LBYAngle = Animstate->m_flGoalFeetYaw; /* Save What We Get From The Server*/
            record->ResolveInfo.Original.ABSAngles = entity->getAbsAngle();
            record->ResolveInfo.Original.EyeAngles = entity->eyeAngles();
            record->ResolveInfo.Original.Set = true;
        }

        entity->UpdateState(Animstate, entity->eyeAngles());
    }
    else {
        if (record->ResolveInfo.CurrentSet.Set) {
            if (hasDesyncHadTimeToDesync || hasDesyncHadTimeToLBY) {
                Animstate->m_flGoalFeetYaw = record->ResolveInfo.CurrentSet.LBYAngle; /* Restore */
                entity->getAbsAngle() = record->ResolveInfo.CurrentSet.ABSAngles;
                entity->eyeAngles() = record->ResolveInfo.CurrentSet.EyeAngles;
                entity->UpdateState(Animstate, entity->eyeAngles());
            }
        }
    }


    if (!(hasDesyncHadTimeToDesync || hasDesyncHadTimeToLBY) && PresumedNoDesync) {
        ResolverDebug(std::wstring{ L"[Resolver.cpp:459] Exiting, Presumed No Desync [" + name + L"] -> " + std::to_wstring(memory->globalVars->currenttime) });
        record->FiredUpon = false;
        return;
    }

    if (!record->ResolveInfo.Original.Set) {
        record->FiredUpon = false;
        ResolverDebug(std::wstring{ L"[Resolver.cpp:392] Exiting, ResolveInfo.Original.Set == false [" + name + L"] -> " + std::to_wstring(memory->globalVars->currenttime)});
        return;
    }

    float DesyncAng = 0.0f;

    if ((record->lbyUpdated && (AnimLayerThree->cycle < .9f) && IN_LBY_ACT) && !record->FiredUpon){
        DesyncAng += (record->totalshots % 3) ? 0 : (record->totalshots % 2) ? -58 : 58;
        Vector ABS = record->ResolveInfo.Original.ABSAngles;
        //Vector EyeAngles = record->ResolveInfo.Original.EyeAngles;
        float LBYAngles = record->ResolveInfo.Original.LBYAngle;
        ABS.y += DesyncAng;
        //EyeAngles.y += DesyncAng;
        LBYAngles += DesyncAng;
        entity->setAbsAngle(ABS);
        //entity->eyeAngles() = EyeAngles;
        Animstate->m_flCurrentFeetYaw = LBYAngles;
        record->lbybackup.Prev = true;
        record->ResolveInfo.wasUpdated = true;
        entity->UpdateState(Animstate, entity->eyeAngles());
        if (record->wasTargeted) {
            Debug::LogItem item;
            item.PrintToScreen = false;           
            //item.text.push_back(L" ");
            //item.text.push_back(L" ");
            item.text.push_back(std::wstring{ L"[Resolver] Assumed LBY Update for " + name + L" at current time " + std::to_wstring(memory->globalVars->currenttime) + L" setting Angles to " + std::to_wstring(DesyncAng) });
            Debug::LOG_OUT.push_front(item);
        }

        return;
    }


    float LBYAng = 0.0f;

    if (!record->lbyUpdated || ((AnimLayerThree->cycle > .9f) && IN_LBY_ACT) || !IN_LBY_ACT || record->FiredUpon) { /* Simple Little Brute Force Method */
        record->FiredUpon = false;
        if ((AnimLayerThree->cycle < .9f) && hasDesyncHadTimeToLBY) {
            LBYAng = (record->totalshots % 2) ? 58 : 0;

            switch (record->missedshots) {
            case 0:
                DesyncAng += 58;
                break;
            case 1:
                DesyncAng += -58.f;
                break;
            case 2:
                DesyncAng = 0;
                break;
            case 3:
                DesyncAng -= 29.0f;
                break;
            case 4:
                DesyncAng += 29.0f;
                record->missedshots = 0;
                break;
            default:
                DesyncAng += 0;
                record->missedshots = 0;
                break;
            }
            DesyncAng += DesyncAng;

        } else if (move) {
            switch ((record->totalshots%2)) {
            case 0:
                DesyncAng -= 58;
                break;
            case 1:
                DesyncAng += 58;
                break;
            }
            DesyncAng *= DesyncFrac;




        } else  { /*Micromoving(?), leading me to believe they are not Extended LBY breaking, and possibly sub Max Desync*/
            switch (record->missedshots) {
            case 0:
                DesyncAng -= 58;
                break;
            case 1:
                DesyncAng += 58;
                break;
            case 2:
                DesyncAng = 0;
                break;
            case 3:
                DesyncAng -= (58 / 2);
                break;
            case 4:
                DesyncAng += (58 / 2);
                break;
            case 5:
                if (!IN_LBY_ACT) {
                    DesyncAng -= 58;
                    record->missedshots = 1;
                    break;
                }
                DesyncAng += 116;         
                break;
            case 6:
                DesyncAng -= 116;
                record->missedshots = 0;
                break;
            default:
                DesyncAng += 0;
                record->missedshots = 0;
                break;
            }

        }

        if (DesyncAng == record->ResolveInfo.ResolveAddition.LBYAngle) {
            if (record->wasTargeted) {
                Debug::LogItem item;
                item.PrintToScreen = false;
                if (config->debug.movefix) {
                    item.text.push_back(std::wstring{ L"Exiting [467] Desync Ang: [" + std::to_wstring(DesyncAng) + L"] ResolveInfo.ResolveAddition.LBYAngle: [" + std::to_wstring(record->ResolveInfo.ResolveAddition.LBYAngle) + L"]" });
                    Debug::LOG_OUT.push_front(item);
                }
                return;
            }
            else {
                if (config->debug.movefix) {
                    std::wstring name = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(entity->getPlayerName(true));
                    ResolverDebug(std::wstring{ L"Exiting [467] Desync Ang: [" + std::to_wstring(DesyncAng) + L"] ResolveInfo.ResolveAddition.LBYAngle: [" + std::to_wstring(record->ResolveInfo.ResolveAddition.LBYAngle) + L"] for entity " + name });
                }
                return;
            }
        }

        Animstate->m_flGoalFeetYaw = record->ResolveInfo.Original.LBYAngle; /* Restore */
        entity->setAbsAngle(record->ResolveInfo.Original.ABSAngles);
        entity->eyeAngles() = record->ResolveInfo.Original.EyeAngles;

        entity->UpdateState(Animstate, entity->eyeAngles());

        record->ResolveInfo.ResolveAddition.ABSAngles = { 0, DesyncAng, 0 };
        record->ResolveInfo.ResolveAddition.EyeAngles = { 0, 0, 0 };//= { 0, DesyncAng, 0 };
        record->ResolveInfo.ResolveAddition.LBYAngle = DesyncAng;

        record->ResolveInfo.CurrentSet.Set = true;
        record->ResolveInfo.CurrentSet.LBYAngle = Animstate->m_flGoalFeetYaw + record->ResolveInfo.ResolveAddition.LBYAngle; /* Set Current Angles */
        record->ResolveInfo.CurrentSet.ABSAngles = (entity->getAbsAngle() + record->ResolveInfo.ResolveAddition.ABSAngles);
        record->ResolveInfo.CurrentSet.EyeAngles = entity->eyeAngles() + record->ResolveInfo.ResolveAddition.EyeAngles; ;

        Animstate->m_flGoalFeetYaw = record->ResolveInfo.CurrentSet.LBYAngle; /* Add on the Additions */
        entity->setAbsAngle(record->ResolveInfo.CurrentSet.ABSAngles);
        entity->eyeAngles() = record->ResolveInfo.CurrentSet.EyeAngles;

        entity->UpdateState(Animstate, entity->eyeAngles());

        Debug::LogItem item;
        item.PrintToScreen = false;


        std::wstring name = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(entity->getPlayerName(true));
        //item.text.push_back(L" ");
        //item.text.push_back(L" ");
        if (config->debug.resolverDebug) {
            item.text.push_back(std::wstring{ L"[Resolver] Attempting to Resolve " + name + L" setting (Y) Angles to " + std::to_wstring(record->ResolveInfo.CurrentSet.EyeAngles.y) + L" Offset " + std::to_wstring(DesyncAng) + L" Original LBY/EyeAngles Were " + std::to_wstring(record->ResolveInfo.Original.LBYAngle) + L"/" + std::to_wstring(record->ResolveInfo.Original.LBYAngle) });
            std::wstring lby_str = std::wstring{ ((record->lbyUpdated) ? L" We Believe LBY Was Updated " : L" We Believe LBY Was Not Updated ") };
            item.text.push_back(std::wstring{ L"[Resolver]" + lby_str + L"at current time " + std::to_wstring(memory->globalVars->currenttime) });
            std::wstring micromove_str = std::wstring{ (AnimLayerThree->cycle > .9f) ? L"[Resolver] Players Cycle is >.9, Assumed to Be Micro-Moving" : L"[Resolver] Players Cycle is <.9, Assumed to not Be Micro-Moving, extended LBY-Desync?" };
            item.text.push_back(micromove_str);
            Debug::LOG_OUT.push_front(item);
        }

        if (config->debug.ResolverOut.enabled) {
            Debug::LogItem item2;
            item2.PrintToConsole = false;

            /*
            
                Resolver Flags:

                MM = Micro-Move
                EL = Extended LBY

                (LBY THIS TICK)
                UL = Updated LBY
                NU = No Updated LBY
            
                M: Moving (No to less desync)
                S: Still

                DT: Has had time to Desync
                ND: Has not had time to Desync

                LT: Has had time to break-LBY
                NL: Has not had time to break-LBY

                LA: In 979 Animation Act
                NA: Not in 979 Animation Act
            
            */



            std::wstring micromove_str = std::wstring{ (AnimLayerThree->cycle > .9f) ? L"MM:" : L"EL:"  };
            std::wstring lby_str = std::wstring{ ((record->lbyUpdated) ? L"UL:" : L"NU:")               };
            std::wstring move_str = std::wstring{ ((move) ? L"M:" : L"S:")                              };
            std::wstring DesyT = std::wstring{ ((hasDesyncHadTimeToDesync) ? L"DT:" : L"ND:")           };
            std::wstring LBYT = std::wstring{ ((hasDesyncHadTimeToLBY) ? L"LT:" : L"NL:")               };
            std::wstring LBYA = std::wstring{ ((IN_LBY_ACT) ? L"LA" : L"NA")                            };




            item2.text.push_back(std::wstring{ L"[Resolver] Attempting To Resolve " + name + L" With Angles: " + std::to_wstring(DesyncAng) + L" " + micromove_str + lby_str + move_str + DesyT + LBYT + LBYA});
            item2.color = { (int)config->debug.ResolverOut.color[0] * 255,(int)config->debug.ResolverOut.color[1] * 255, (int)config->debug.ResolverOut.color[2] * 255 };
            Debug::LOG_OUT.push_front(item2);
        }
        record->ResolveInfo.wasUpdated = true;
        record->FiredUpon = false;
        return;
    }
    else {
        Debug::LogItem item;
        item.PrintToScreen = false;
        item.text.push_back(std::wstring{ L"Error [Resolver.cpp:511]: Exiting, no conditions met for resolve" });
        Debug::LOG_OUT.push_front(item);
        record->FiredUpon = false;
    }


    record->FiredUpon = false;

    ResolverDebug(std::wstring{ L"[Resolver.cpp:542] Exiting, End of function" });

    






    // __MAKE INTO FUNCTION__




}



/*
void Resolver::BasicResolver(Entity* entity, int missed_shots) {

    if (!config->debug.animstatedebug.resolver.basicResolver)
        return;

    config->debug.indicators.resolver = false;
    if (!localPlayer || !localPlayer->isAlive() || !entity || entity->isDormant() || !entity->isAlive())
        return;

    if (PlayerRecords.empty())
        return;

    auto record = &PlayerRecords.at(entity->index());

    if (!record || record->invalid)
        return;

    if (entity->getVelocity().length2D() > 5.0f) {
        record->PreviousEyeAngle = entity->eyeAngles().y;
        return;
    }

    auto b_record = Backtrack::records[entity->index()];
    if ((b_record.size() > 0) && !b_record.empty() && Backtrack::valid(b_record.front().simulationTime))
    {
        if (b_record.front().onshot)
            return;
    }


    auto Animstate = entity->getAnimstate();

    if (!Animstate)
        return;


    bool lby = record->lbyUpdated;

    if (!record->FiredUpon || !record->wasTargeted) { // || !record->wasTargeted
        for (int b = 0; b < entity->getAnimationLayerCount(); b++) {
            auto AnimLayer = entity->getAnimationLayer(b);

            if (entity->getSequenceActivity(AnimLayer->sequence) == ACT_CSGO_FIRE_PRIMARY && (AnimLayer->weight > (0.5f)) && (AnimLayer->cycle < .8f)) {
                return;
            }

        };

        entity->UpdateState(Animstate, entity->eyeAngles());
        if ((record->wasUpdated == false) && (entity->eyeAngles().y != record->PreviousEyeAngle) && (record->prevSimTime != entity->simulationTime())) {
            record->originalLBY = Animstate->m_flGoalFeetYaw;
            //record->PreviousEyeAngle = entity->eyeAngles().y;
            record->eyeAnglesOnUpdate = entity->eyeAngles().y;
            record->prevSimTime = entity->simulationTime();
            record->PreviousEyeAngle = entity->eyeAngles().y + (record->PreviousEyeAngle - record->PreviousDesyncAng);
            record->wasUpdated = true;
        } 





        
        if ((record->wasUpdated == true) && (entity->eyeAngles().y != record->PreviousEyeAngle) && (record->prevSimTime != entity->simulationTime())){
            //record->PreviousEyeAngle = entity->eyeAngles().y;

        }
        


        if (lby) {
            record->lbybackup.AbsAngle = entity->getAbsAngle();
            record->lbybackup.EyeAngle = entity->eyeAngles().y;
            record->lbybackup.Prev = true;
        } else if (!lby) {
            if (record->lbybackup.Prev == true) {
                record->lbybackup.Prev = false;
                entity->setAbsAngle(record->lbybackup.AbsAngle);
                entity->eyeAngles().y = record->lbybackup.EyeAngle;
                entity->UpdateState(Animstate, entity->eyeAngles());
            }
        }

        entity->eyeAngles().y = record->PreviousEyeAngle;
        entity->UpdateState(Animstate, entity->eyeAngles());

        
    }


    float DesyncAng = record->originalLBY;

    //if (record->wasUpdated == true)
    //   return;

    entity->UpdateState(Animstate, entity->eyeAngles());

    config->debug.indicators.resolver = true;

    int missed = record->missedshots;

    if (config->debug.animstatedebug.resolver.missed_shots > 0) {
        missed = config->debug.animstatedebug.resolver.missed_shots;
    }

    if (record->lastworkingshot != -1) {
        missed = record->lastworkingshot;
        record->lastworkingshot = -1;
    }

    if (!lby) {
        switch (missed) {
        case 1: // POP all those pasted Polak AA's
            if (record->originalLBY == 0) {
                record->originalLBY = Animstate->m_flGoalFeetYaw;
            }
            DesyncAng = record->originalLBY;
            break;
        case 2:
            DesyncAng += 58.0f;
            break;
        case 3:
            DesyncAng -= 58.0f;
            break;
        case 4:
            DesyncAng += 29.0f;
            break;
        case 5:
            DesyncAng -= 29.0f;
            break;
        case 7:
            DesyncAng += -116.0f;
            break;
        case 8:
            DesyncAng += +116.0f;
            break;
  DesyncAng += 15;
        default:
            DesyncAng += 5;
            record->missedshots = 0;
            break;
        }
    }
    else {
        DesyncAng += (record->totalshots % 3) ? 0 : (record->totalshots % 2) ? -(entity->getMaxDesyncAngle()) : (entity->getMaxDesyncAngle());
    }


    if (DesyncAng > 180) {
        float off = DesyncAng - 180;
        DesyncAng = -180 + off;
    }
    else  if (DesyncAng < -180) {
        DesyncAng = (DesyncAng + 180) + 180;
    }

    Debug::LogItem item;
    item.PrintToScreen = false;


    //std::wstring name = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(entity->getPlayerName(true));
    //item.text.push_back(std::wstring{ L"[Resolver] Attempting to Resolve " + name + L" setting (Y) Angles to " + std::to_wstring(DesyncAng) + L" Offset " + std::to_wstring(DesyncAng - record->originalLBY) + L" Original LBY/EyeAngles Were " + std::to_wstring(record->originalLBY) + L"/" + std::to_wstring(record->PreviousEyeAngle)});
    //std::wstring lby_str = std::wstring{ ((lby) ? L" We Believe LBY Was Updated " : L" We Believe LBY Was Not Updated ") };
    //item.text.push_back(std::wstring{L"[Resolver]" + lby_str + L"at current time " + std::to_wstring(memory->globalVars->currenttime)});


    entity->eyeAngles().y = DesyncAng;
    entity->setAbsAngle(entity->eyeAngles());

    //record->missedshots = missed;

    Animstate->m_flGoalFeetYaw = DesyncAng;

    entity->UpdateState(Animstate, entity->eyeAngles());


        std::wstring name = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(entity->getPlayerName(true));
        item.text.push_back(std::wstring{ L"[Resolver] Attempting to Resolve " + name + L" setting (Y) Angles to " + std::to_wstring(DesyncAng) + L" Offset " + std::to_wstring(DesyncAng - record->originalLBY) + L" Original LBY/EyeAngles Were " + std::to_wstring(record->originalLBY) + L"/" + std::to_wstring(record->PreviousEyeAngle) });
        std::wstring lby_str = std::wstring{ ((lby) ? L" We Believe LBY Was Updated " : L" We Believe LBY Was Not Updated ") };
        item.text.push_back(std::wstring{ L"[Resolver]" + lby_str + L" at current time " + std::to_wstring(memory->globalVars->currenttime) });

        if (!lby) {
            record->PreviousEyeAngle = entity->eyeAngles().y;

            if (record->PreviousEyeAngle > 180) {
                float off = record->PreviousEyeAngle - 180;
                record->PreviousEyeAngle = -180 + off;
            }
            else  if (DesyncAng < -180) {
                record->PreviousEyeAngle = (record->PreviousEyeAngle + 180) + 180;
            }
            record->PreviousDesyncAng = DesyncAng;
            record->wasTargeted = true;
            record->FiredUpon = false;
            record->wasUpdated = true;
        }
        
    Debug::LOG_OUT.push_front(item);
    return;

}
*/