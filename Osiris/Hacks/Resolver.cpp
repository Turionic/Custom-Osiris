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

#include <deque>

const int MAX_RECORDS = 128;

std::vector<Resolver::Record> Resolver::PlayerRecords(65);
Resolver::Record Resolver::invalid_record;
Entity* Resolver::TargetedEntity;

void Resolver::CalculateHits(GameEvent event) {
    /* Calculate Spread Misses
       Calculate Hitbox hit
       Calculate Damage Done
       Whether to inc missed shots
       etc 
     */



}




bool Resolver::LBY_UPDATE(Entity* entity, Resolver::Record* record) {

    bool returnval = false;
    float servertime = memory->globalVars->serverTime();

    if (record->velocity > 0.1f) { //LBY updates on any velocity
        record->lbyNextUpdate = 0.22f + servertime;
        return false;
    }

    if (record->lbyNextUpdate < servertime) {
        record->lbyNextUpdate = servertime + 1.1f;
        returnval = true;
    }
    else {
        returnval = false;
    }


    return returnval;
    
}



void Resolver::Update() {

    for (int i = 0; i < PlayerRecords.size(); i++) {
        if (!localPlayer || !localPlayer->isAlive()) {
            //return;
            auto record = &PlayerRecords.at(i);
            if (!record)
                continue;
            record->invalid = true;
            record->shots.clear();
            continue;
        }

        auto record = &PlayerRecords.at(i);

        if (!record)
            continue;

        auto entity = interfaces->entityList->getEntity(i);

        if (!entity || entity->isDormant() || !entity->isAlive() || ((entity != localPlayer.get()) && !entity->isOtherEnemy(localPlayer.get()))) {
            if (record->FiredUpon || record->wasTargeted) {
                record->lastworkingshot = record->missedshots;
            }
            record->invalid = true;
            //record->shots.clear();
            continue;
        }

        record = &PlayerRecords.at(entity->index());

        record->prevVelocity = record->velocity;
        record->velocity = entity->getVelocity().length2D();
        if (record->invalid == true) {
            record->missedshots = config->debug.animstatedebug.resolver.missedoffset;
            record->prevhealth = entity->health();
            record->wasTargeted = false;
            record->FiredUpon = false;
            record->multiExpan = 2.0f;
        }
        
        record->lbyUpdated = LBY_UPDATE(entity, record);
        //record->activeweapon = entity->getActiveWeapon();

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
        else if (((record->prevhealth == entity->health()) && !record->invalid) || (config->debug.animstatedebug.resolver.goforkill)) {
            record->lastworkingshot = -1;
            record->missedshots = (record->missedshots >= 8 ? config->debug.animstatedebug.resolver.missedoffset : ++(record->missedshots));
            record->wasUpdated = false;
        }
        else if (!record->shots.empty()) {
            record->shots.back().DamageDone = record->prevhealth - entity->health(); 
        }
        

        record->prevhealth = entity->health();

        record->invalid = false;

        if (record->wasTargeted)
            TargetedEntity = entity;

    }
}

/*
void Resolver::AnimStateResolver(Entity* entity, int missed_shots) {

    // __ MAKE INTO FUNCTION __
    config->debug.indicators.resolver = false;
    if (!localPlayer || !localPlayer->isAlive() || !entity || entity->isDormant() || !entity->isAlive())
        return;

    if (PlayerRecords.empty())
        return;

    auto record = &PlayerRecords.at(entity->index());

    if (!record || record->invalid)
        return;

    if (entity->getVelocity().length2D() > 3.0f) {
        record->PreviousEyeAngle = entity->eyeAngles().y;
        return;
    }


    auto Animstate = entity->getAnimstate();

    if (!Animstate)
        return;

    if (!record->FiredUpon || !record->wasTargeted) {


        for (int b = 0; b < entity->getAnimationLayerCount(); b++) {
            auto AnimLayer = entity->getAnimationLayer(b);

            if (entity->getSequenceActivity(AnimLayer->sequence) == ACT_CSGO_FIRE_PRIMARY && (AnimLayer->weight > (0.5f)) && (AnimLayer->cycle < .8f)) {
                return;
            }

        };

        entity->UpdateState(Animstate, entity->eyeAngles());
        if ((record->wasUpdated == false) && (entity->eyeAngles().y != record->PreviousEyeAngle)) {

            record->eyeAnglesOnUpdate = entity->eyeAngles().y;
            record->prevSimTime = entity->simulationTime();
            record->PreviousEyeAngle = entity->eyeAngles().y + (record->PreviousEyeAngle - record->PreviousDesyncAng);
            record->wasUpdated = true;
        }

        entity->eyeAngles().y = record->PreviousEyeAngle;
        entity->UpdateState(Animstate, entity->eyeAngles());

        return;
    }
    // __MAKE INTO FUNCTION__




}
*/
#include "../Debug.h"
#include <mutex>
#include <numeric>
#include <sstream>
#include <codecvt>
#include <locale>
#include <string>
#include <iostream>
#include <cstddef>
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

     /*
    const auto Brecord = &Backtrack::records[entity->index()];

    if (!(Brecord && Vrecord->size() && Backtrack::valid(Vrecord->front().simulationTime)))
        return;
    */

    /*

	auto Animstate = entity->getAnimstate();

	if (!Animstate)
		return;
    


    int currAct = 1;

    if (entity->getVelocity().length2D() > 1.0f)
        return;

    for (int b = 0; b < entity->getAnimationLayerCount(); b++) {
        auto AnimLayer = entity->getAnimationLayer(b);
        currAct = entity->getSequenceActivity(AnimLayer->sequence);

        //if (currAct == ACT_CSGO_FIRE_PRIMARY && (AnimLayer->weight > (0.0f)) && (AnimLayer->cycle < .8f))
         //   return;, 
          
        if ((currAct == ACT_CSGO_IDLE_TURN_BALANCEADJUST) || (currAct == ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING)) {
            break;
        }
    };

    float DesyncAng = Animstate->m_flGoalFeetYaw;
    //float eyeAnglesY = entity->eyeAngles().y;


   //record->front().prevAngles = entity->eyeAngles().y;

    if ((record->size() > 1) && Backtrack::valid(record->at(1).simulationTime)) {
        if (record->at(1).wasUpdated) {
                entity->eyeAngles().y = record->front().resolvedaddition;
                Animstate->m_flGoalFeetYaw = record->front().resolvedaddition;
                entity->UpdateState(Animstate, entity->eyeAngles());
                return;
        }

    }

    if (!currAct)
        return;

    if ((currAct != ACT_CSGO_IDLE_TURN_BALANCEADJUST) && (currAct != ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING) && !(config->debug.animstatedebug.resolver.overRide)) {
        return;
    }


    if (  (   !(record->front().PreviousAct == ACT_CSGO_IDLE_TURN_BALANCEADJUST) && !(record->front().PreviousAct == ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING)  ) && !(config->debug.animstatedebug.resolver.overRide)) {
        return;
    }
    
    

    //if ((record->front().PreviousAct == 0) || (!record->front().wasTargeted))
    //    return;

   //record->front().PreviousAct = 0;
    */



    auto Animstate = entity->getAnimstate();

    if (!Animstate)
        return;




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
            /*
            record->eyeAnglesOnUpdate = entity->eyeAngles().y;
            record->PreviousEyeAngle = entity->eyeAngles().y + record->PreviousDesyncAng;
            record->prevSimTime = entity->simulationTime();
            */
        }
        
        entity->eyeAngles().y = record->PreviousEyeAngle;
        entity->UpdateState(Animstate, entity->eyeAngles());

        return;
    }
    
    float DesyncAng = record->originalLBY;

    //if (record->wasUpdated == true)
    //   return;

    entity->UpdateState(Animstate, entity->eyeAngles());

    config->debug.indicators.resolver = true;

    int missed = record->missedshots;

    if (config->debug.animstatedebug.resolver.missed_shots > 0)
        missed = config->debug.animstatedebug.resolver.missed_shots;

    if (record->lastworkingshot != -1) {
        missed = record->lastworkingshot;
        record->lastworkingshot = -1;
    }








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
        /*
        case 10:
            DesyncAng = DesyncAng + (entity->getMaxDesyncAngle() * -1);
            break;
        case 11:
            DesyncAng = DesyncAng + entity->getMaxDesyncAngle();
            break;
        */
       // case 8:
        //    DesyncAng += -15;
       // case 9:
       //     DesyncAng += 15;
        default:
            DesyncAng += 5;
            record->missedshots = 0;
            break;
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
    std::wstring name = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(entity->getPlayerName(true));
    item.text.push_back(std::wstring{ L"Attempting to Resolve " + name + L" setting (Y) Angles to " + std::to_wstring(DesyncAng) + L" Offset " + std::to_wstring(DesyncAng - record->originalLBY) + L" Original LBY/EyeAngles Were " + std::to_wstring(record->originalLBY) + L"/" + std::to_wstring(record->PreviousEyeAngle)});
    Debug::LOG_OUT.push_front(item);

    entity->eyeAngles().y = DesyncAng;
    entity->setAbsAngle(entity->eyeAngles());

    //record->missedshots = missed;
    Animstate->m_flGoalFeetYaw = DesyncAng;

    entity->UpdateState(Animstate, entity->eyeAngles());
    record->PreviousEyeAngle = entity->eyeAngles().y;

    if (record->PreviousEyeAngle > 180) {
        float off = record->PreviousEyeAngle - 180;
        record->PreviousEyeAngle = -180 + off;
    }
    else  if (DesyncAng < -180) {
        record->PreviousEyeAngle = (record->PreviousEyeAngle + 180) + 180;
    }

    record->PreviousDesyncAng = DesyncAng;
    record->wasUpdated = true;
    record->wasTargeted = true;
    record->FiredUpon = false;
    //record->front().resolvedaddition = entity->eyeAngles().y;
    //Animstate->m_flEyeYaw = Animstate->m_flCurrentFeetYaw;
    //Animstate->m_flGoalFeetYaw = 0.0f;
    //entity->eyeAngles().x = Animstate->m_flPitch;

    return;

}