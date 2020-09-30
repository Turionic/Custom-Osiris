#include <mutex>
#include <numeric>
#include <sstream>
#include <codecvt>
#include <locale>

#include "Debug.h"

#include "Config.h"
#include "Interfaces.h"
#include "Memory.h"
#include "Netvars.h"
#include "./Hacks/Misc.h"
#include "./SDK/ConVar.h"
#include "./SDK/Surface.h"
#include "./SDK/GlobalVars.h"
#include "./SDK/NetworkChannel.h"
#include "./SDK/WeaponData.h"
#include "./Hacks/EnginePrediction.h"
#include "./SDK/LocalPlayer.h"
#include "./SDK/Entity.h"
#include "./SDK/UserCmd.h"
#include "./SDK/GameEvent.h"
#include "./SDK/FrameStage.h"
#include "./SDK/Client.h"
#include "./SDK/ItemSchema.h"
#include "./SDK/WeaponSystem.h"
#include "./SDK/WeaponData.h"
#include "./GUI.h"
#include "./Helpers.h"
#include "./SDK/ModelInfo.h"
#include "./Hacks/Resolver.h"
#include "./Hacks/Backtrack.h"

#include <vector>

/*

struct debug{
	ColorToggle desync_info;



} Debug; 






*/
/*
struct screen {
	int Width;
	int Height;
	int CurrPosW = 0;
	int CurrPosH = 5;
	int CurrColumnMax;

} Screen;

struct ColorInfo {
	bool enabled;
	float r;
	float g;
	float b;
	float a;
} ColorInf;
*/


/* OUTPUT */
Debug::screen Debug::Screen;


void Debug::DrawBox(coords start, coords end) {
    if ((start.y > end.y) || (end.y > Screen.Height) || (start.y < 0))
        return;
    if ((start.x > end.x) || (end.x > Screen.Width) || (start.x < 0))
       return;

    interfaces->surface->setDrawColor(config->debug.box.color[0], config->debug.box.color[1], config->debug.box.color[2], 180);
    interfaces->surface->drawFilledRect(start.x, start.y, end.x, end.y);


}
#include <deque>

std::deque<Debug::LogItem> Debug::LOG_OUT;
#include "fnv.h"

/*

#define	HITGROUP_GENERIC	0
#define	HITGROUP_HEAD		1
#define	HITGROUP_CHEST		2
#define	HITGROUP_STOMACH	3
#define HITGROUP_LEFTARM	4
#define HITGROUP_RIGHTARM	5
#define HITGROUP_LEFTLEG	6
#define HITGROUP_RIGHTLEG	7


*/

static std::wstring HitGroups[] = {
    L" Generic ",
    L" Head ",
    L" Chest ",
    L" Stomach ",
    L" Left Arm ",
    L" Right Arm ",
    L" Left Leg ",
    L" Right Leg ",
    L" Gear " // lol
};

void Debug::Logger(GameEvent *event) {
    if (!localPlayer || localPlayer->isDormant() || !localPlayer->isAlive())
        return;

    Entity* attacker = interfaces->entityList->getEntity(interfaces->engine->getPlayerForUserID(event->getInt("attacker")));

    if(!attacker || (attacker != localPlayer.get()))
        return;

    uint32_t eventHash = fnv::hashRuntime(event->getName());

    Entity* player = interfaces->entityList->getEntity(interfaces->engine->getPlayerForUserID(event->getInt("userid")));
    std::wstring playerName = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(player->getPlayerName(true));

    //interfaces->entityList->
    if (eventHash = fnv::hash("player_hurt")) {

        short health = event->getInt("dmg_health");
        //short armor = event->getInt("dmg_armor");
        int r_health = event->getInt("health");
        //int r_armor = event->getInt("armor");
        std::wstring weapon = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(event->getString("weapon"));

        Debug::LogItem logEntry;
        std::wstring text = {L"Shot " + playerName + L" for " + std::to_wstring(health) + L"hp in the" + HitGroups[event->getInt("hitgroup")] + L"with a " + weapon + L" || " + std::to_wstring(r_health) + L"hp remaining"};
        logEntry.text.push_back(text);
        logEntry.time_of_creation = memory->globalVars->realtime;

        LOG_OUT.push_front(logEntry);

    }
    else if (eventHash = fnv::hash("player_death")) { // TODO: Make this lol
        return;
        Debug::LogItem logEntry;
        std::wstring weapon = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(event->getString("weapon"));
        
        std::wstring Headshot = L" ";

    
        logEntry.time_of_creation = memory->globalVars->currenttime;
        LOG_OUT.push_front(logEntry);
    
    }





}

template <class T>
static void EraseDeque(std::deque<T>& de, int offset) {
    for (int i = de.size(); i > offset; i--) {
        de.pop_back();
    }
}

void Debug::PrintLog() {
    if (LOG_OUT.size() > 8)
        EraseDeque(LOG_OUT, 8);


    int i = 0;
    for (; i < LOG_OUT.size(); i++) {
        LogItem item = LOG_OUT.at(i);
        if ((item.time_of_creation + 10) < (memory->globalVars->realtime)) { EraseDeque(LOG_OUT, i); return; }
        interfaces->surface->setTextFont(Surface::font);
        interfaces->surface->setTextColor(255, 255, 255, (1 - (((memory->globalVars->realtime - item.time_of_creation)) / 10.0f)) * 255);
        //SetupTextPos
        Draw_Text(item.text);
    }


}






void Debug::DrawGraphBox(coords start, coords end, float min_val, float max_val, float val, float ratio, std::wstring name) {

    interfaces->surface->setDrawColor(config->debug.box.color[0], config->debug.box.color[1], config->debug.box.color[2], 180);
    interfaces->surface->drawOutlinedRect(start.x, start.y, end.x, end.y);
    int print_val = val;
    if (val > max_val) {
        val = max_val;
    }
    else if (val < min_val) {
        val = min_val;
    }

    if (min_val < 1) {
        min_val = 1;
        max_val += (-1 * (0 - min_val));
        val += (-1 * (0 - min_val));
    }

    if (val < 1)
        return;

    int height = static_cast<int>((end.y - start.y) * (val / (max_val - min_val)));
    start.y = end.y - height;

    if ((start.y > end.y) || (end.y > Screen.Height) || (start.y < 0))
        return;
    if ((start.x > end.x) || (end.x > Screen.Width) || (start.x < 0))
        return;


    interfaces->surface->setDrawColor(config->debug.box.color[0], config->debug.box.color[1], config->debug.box.color[2], 255);
    interfaces->surface->drawFilledRect(start.x, start.y, end.x, end.y);

    std::wstring fps{ std::to_wstring(static_cast<int>(print_val))};
    const auto [fpsWidth, fpsHeight] = interfaces->surface->getTextSize(Surface::font, fps.c_str());
    interfaces->surface->setTextColor(config->debug.animStateMon.color);
    interfaces->surface->setTextPosition(start.x, (start.y - fpsHeight));
    interfaces->surface->printText(fps.c_str());

    interfaces->surface->setTextColor(config->debug.animStateMon.color);
    interfaces->surface->setTextPosition(start.x, end.y);
    interfaces->surface->printText(name.c_str());


}


bool Debug::SetupTextPos(std::vector <std::wstring>& Text) { // If true, can't draw anymore due to screen being full

	int pos_inc_h = 0;
	int pos_inc_w = 0;
    int textsize = 0;
    for (int i = 0; i < Text.size(); i++) {

        std::wstring Str = Text[i];
		const auto [text_size_w, text_size_h] = interfaces->surface->getTextSize(Surface::font, Str.c_str());
        textsize = text_size_h;
		pos_inc_h += text_size_h;
		pos_inc_w = text_size_w;

        if (Screen.CurrColumnMax < pos_inc_w)
            Screen.CurrColumnMax = pos_inc_w;
	}


	


	if ((pos_inc_h + Screen.CurrPosH) > Screen.Height) {
        Screen.CurrPosH = 5;
        Screen.CurrPosW += Screen.CurrColumnMax + 1;
        Screen.CurrColumnMax = 0;
        pos_inc_h = 0;
	}


	if ((pos_inc_w + Screen.CurrPosW) > Screen.Width) {
		return true;
	}

    if (config->debug.box.enabled) {
        coords start;
        coords end;
        start.x = Screen.CurrPosW;
        start.y = Screen.CurrPosH + textsize;
        end.x = start.x + Screen.CurrColumnMax;
        end.y = start.y + pos_inc_h;
        Debug::DrawBox(start,end);
    }

	return false;

}

void Debug::Draw_Text(std::vector<std::wstring>& Text) {


    //std::wstring lolstr = { L"CurrPosH " + std::to_wstring(Screen.CurrPosH) + L" CurrPosW " + std::to_wstring(Screen.CurrPosW) };
    //interfaces->surface->setTextPosition(110, 110);
    //interfaces->surface->printText(lolstr.c_str());

    if (SetupTextPos(Text)) {
        return;
    }


    for (int i = 0; i < Text.size(); i++) {

        std::wstring Str = Text[i];

		const auto [text_size_w, text_size_h] = interfaces->surface->getTextSize(Surface::font, Str.c_str());
		Screen.CurrPosH += text_size_h;
		interfaces->surface->setTextPosition(Screen.CurrPosW, Screen.CurrPosH);
		interfaces->surface->printText(Str.c_str());
	}
}

#include "Hacks/AntiAim.h"
void Debug::AnimStateMonitor() noexcept
{


    for (int i = 1; i <= interfaces->engine->getMaxClients(); i++) {


        auto entity = interfaces->entityList->getEntity(i);

        if ((i != config->debug.entityid) && (config->debug.entityid != -1))
            continue;

        if (!entity || entity->isDormant() || !entity->isAlive() || !entity->isPlayer())
            continue;

        auto AnimState = entity->getAnimstate();
        if (!AnimState)
            continue;


        for (int b = 0; b < entity->getAnimationLayerCount(); b++) {

            if (config->debug.overlay > 15)
                config->debug.overlay = 0;
            else if (config->debug.overlay < 0)
                config->debug.overlay = 15;

            if ((config->debug.overlay > 0) && (config->debug.overlay != b))
                continue;

            auto AnimLayer = entity->getAnimationLayer(b);

            if (!AnimLayer)
                continue;

            auto model = entity->getModel();
            if (!model)
                continue;

            auto studiohdr = interfaces->modelInfo->getStudioModel(model);
            if (!studiohdr)
                continue;

            int Act = entity->getSequenceActivity(AnimLayer->sequence);

            //if (Act == 0 || Act == -1)
           //    continue;


            //if (config->debug.overlayall) {
            //    if (Act != 979)
            //        continue;
            //}


            if (config->debug.weight) {
                if (!(AnimLayer->weight >= 1))
                    continue;
            }


            interfaces->surface->setTextColor(config->debug.animStateMon.color);

            std::wstring AnimStateStr;

            AnimActs animact;

            AnimStateStr = (animact.getEnum_Array()[Act] + L" " + std::to_wstring(Act));
            std::wstring sequence{ L"Sequence: " + std::to_wstring(AnimLayer->sequence) };
            std::wstring playername{ L"PlayerName: " + std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(entity->getPlayerName(true)) };
            std::wstring clientblend{ L"Client Blend: " + std::to_wstring(AnimLayer->clientblend)};
            std::wstring dispatchedsrc{ L"Dispatch Source: " + std::to_wstring(AnimLayer->dispatchedsrc) };
            std::wstring dispatcheddst{ L"Dispatch Source: " + std::to_wstring(AnimLayer->dispatcheddst) };
            std::wstring blendin{ L"blendin: " + std::to_wstring(AnimLayer->blendin) };
            std::wstring weight{ L"Weight: " + std::to_wstring(AnimLayer->weight) };
            std::wstring weightdelta{ L"Weight Delta Rate: " + std::to_wstring(AnimLayer->weightdeltarate) };
            std::wstring cycle{ L"Cycle: " + std::to_wstring(AnimLayer->cycle) };
            std::wstring prevcycle{ L"Previous Cycle " + std::to_wstring(AnimLayer->prevcycle) };
            std::wstring overlay{ L"Overlay: " + std::to_wstring(b) };
            std::wstring state{ L"AnimState: " + AnimStateStr };
            std::wstring order{ L"Order: " + std::to_wstring(AnimLayer->order) };
            std::wstring playbackrate{ L"Playback Rate: " + std::to_wstring(AnimLayer->playbackRate) };





            std::vector<std::wstring> strings{ playername, clientblend, dispatchedsrc, dispatcheddst, blendin, cycle, prevcycle, weight, weightdelta, overlay, state, sequence, order, playbackrate };

            if (config->debug.AnimExtras) {
                std::wstring currPitch{ L"Current Pitch: " + std::to_wstring(AnimState->m_flPitch) };
                std::wstring currLBY{ L"Current LBY: " + std::to_wstring(AnimState->m_flGoalFeetYaw) };
                std::wstring currLBYFEETDelta{ L"LBY/Feet Delta: " + std::to_wstring(AnimState->m_flGoalFeetYaw - entity->eyeAngles().y) };
                std::wstring veloY{ L"Velocity Y: " + std::to_wstring(entity->getVelocity().y) };
                std::wstring veloX{ L"Velocity X: " + std::to_wstring(entity->getVelocity().x) };
                std::wstring veloZ{ L"Velocity Z: " + std::to_wstring(entity->getVelocity().z) };
                std::wstring currEyeYawY{ L"Current Eye Y: " + std::to_wstring(entity->eyeAngles().y) };
                std::wstring currEyeYawX{ L"Current Eye X: " + std::to_wstring(entity->eyeAngles().x) };
                std::wstring currEyeYawZ{ L"Current Eye Z: " + std::to_wstring(entity->eyeAngles().z) };
                std::wstring veloAngY{ L"Velocity Angle Y: " + std::to_wstring(entity->getVelocity().toAngle().y) };
                std::wstring veloAngX{ L"Velocity Angle X: " + std::to_wstring(entity->getVelocity().toAngle().x) };
                std::wstring veloAngZ{ L"Velocity Angle Z: " + std::to_wstring(entity->getVelocity().toAngle().z) };
                std::wstring veloang{ L"Velo Angle: " + std::to_wstring(atan2(entity->getVelocity().x, entity->getVelocity().y) * (180/3.14)) };
                std::wstring veloangVersEyeAng{ L"Velo Angle: " + std::to_wstring(atan2(entity->getVelocity().x, entity->getVelocity().y) * (180 / 3.14)) };
                std::wstring eyeveloangdelta{ L"Eye-Velo Delta Ang: " + std::to_wstring(entity->getVelocity().toAngle().y - entity->eyeAngles().y) };

                
                std::wstring originY{ L"Origin Y: " + std::to_wstring(entity->origin().y) };
                std::wstring originX{ L"Origin X: " + std::to_wstring(entity->origin().x) };
                std::wstring originZ{ L"Origin Z: " + std::to_wstring(entity->origin().z) };
                //std::wstring deltaY{ L"Delta Eye-Org Y: " + std::to_wstring(entity->eyeAngles().y - entity->origin().y) };
                //std::wstring deltaX{ L"Delta Eye-Org X: " + std::to_wstring(entity->eyeAngles().x - entity->origin().x) };
                //std::wstring deltaZ{ L"Delta Eye-Org Z: " + std::to_wstring(entity->eyeAngles().z - entity->origin().z) };
                std::wstring absY{ L"ABS Origin Y: " + std::to_wstring(entity->getAbsOrigin().y) };
                std::wstring absX{ L"ABS Origin X: " + std::to_wstring(entity->getAbsOrigin().x) };
                std::wstring absZ{ L"ABS Origin Z: " + std::to_wstring(entity->getAbsOrigin().z) };

                std::wstring absAY{ L"ABS Angle Y: " + std::to_wstring(entity->getAbsAngle().y) };
                std::wstring absAX{ L"ABS Angle X: " + std::to_wstring(entity->getAbsAngle().x) };
                std::wstring absAZ{ L"ABS Angle Z: " + std::to_wstring(entity->getAbsAngle().z) };

                std::wstring absVY{ L"ABS Velocity Y: " + std::to_wstring(entity->getAbsVelocity()->y) };
                std::wstring absVX{ L"ABS Velocity X: " + std::to_wstring(entity->getAbsVelocity()->x) };
                std::wstring absVZ{ L"ABS Velocity Z: " + std::to_wstring(entity->getAbsVelocity()->z) };
                //std::wstring originDistY{ L"Origin LP Transformed Y: " + std::to_wstring(entity->origin().distTo().y) };
                //std::wstring originDistX{ L"Origin LP Transformed X: " + std::to_wstring(entity->origin().distTo().x) };
                //std::wstring originDistZ{ L"Origin LP Transformed Z: " + std::to_wstring(entity->origin().distTo().z) };

                std::wstring obbmin{ L"Obbmin X: " + std::to_wstring(entity->getCollideable()->obbMins().x) };
                std::wstring obbmax{ L"Obbmax X: " + std::to_wstring(entity->getCollideable()->obbMaxs().x) };


                std::wstring speed2D{ L"Speed 2D: " + std::to_wstring(AnimState->speed_2d) };
                

                std::wstring aaup{ L"LBYNEXTUPDATE: " + std::to_wstring(AntiAim::LocalPlayerAA.lbyNextUpdate) };
                std::wstring lastlby{ L"lbylastval: " + std::to_wstring(AntiAim::LocalPlayerAA.lastlbyval) };

                std::wstring servtime{ L"Server Time: " + std::to_wstring(memory->globalVars->serverTime())};
                std::wstring lbyupdate{ L"Will LBY UPDATE: " + std::to_wstring(AntiAim::lbyNextUpdated) };

                const auto record = &Backtrack::records[entity->index()];
                    if(record && record->size() && Backtrack::valid(record->front().simulationTime)){
                        int lastAct = record->back().PreviousAct;
                        if (lastAct) {
                            std::wstring PrevString;
                            if (lastAct == -399 || lastAct == -400 || lastAct == -1 || lastAct > 1000) {
                                PrevString = L"INVALID ";
                            }
                            else {
                                PrevString = animact.getEnum_Array()[lastAct];
                            }

                            AnimStateStr = (PrevString + L" " + std::to_wstring(lastAct));
                            std::wstring prevstate{ L"Previous AnimState: " + AnimStateStr };

                            strings.push_back(prevstate);
                        }

                    }
            
                strings.push_back(currPitch);
                strings.push_back(currLBY);
                strings.push_back(currLBYFEETDelta);
                strings.push_back(speed2D);
                strings.push_back(veloAngX);
                strings.push_back(veloAngY);
                strings.push_back(veloAngZ);
                strings.push_back(veloX);
                strings.push_back(veloY);
                strings.push_back(veloZ);
                strings.push_back(currEyeYawX);
                strings.push_back(currEyeYawY);
                strings.push_back(currEyeYawZ);


                
                //strings.push_back(veloX);
                //strings.push_back(veloY);
                strings.push_back(speed2D);
                //strings.push_back(currEyeYawY);

                strings.push_back(veloang);
                strings.push_back(eyeveloangdelta);

                strings.push_back(originX);
                strings.push_back(originY);
                strings.push_back(originZ);

                //strings.push_back(deltaX);
                //strings.push_back(deltaY);
                //strings.push_back(deltaZ);
                strings.push_back(absX);
                strings.push_back(absY);
                strings.push_back(absZ);

                strings.push_back(absAX);
                strings.push_back(absAY);
                strings.push_back(absAZ);

                strings.push_back(absVX);
                strings.push_back(absVY);
                strings.push_back(absVZ);

                strings.push_back(obbmin);
                strings.push_back(obbmax);

                if (entity == localPlayer.get() && config->antiAim.enabled) {
                    strings.push_back(aaup);
                    strings.push_back(lastlby);
                    strings.push_back(servtime);
                    strings.push_back(lbyupdate);
                   
                }
            }

            if (config->debug.ResolverRecords) {
                auto record = &Resolver::PlayerRecords.at(entity->index());
                if (!record || record->invalid == true)
                    return;

                if (config->debug.TargetOnly) {
                    if (!record->wasTargeted)
                        continue;
                }

                std::wstring Resolver{ L"RESOLVER INFO: " };
                std::wstring simTime{ L"Simulation Time: " + std::to_wstring(record->prevSimTime) };
                std::wstring wasTargeted{ L"Was Targeted: " + std::to_wstring(record->wasTargeted) };
                std::wstring FiredUpon{ L"Fired Upon: " + std::to_wstring(record->FiredUpon) };
                std::wstring WasUpdated{ L"Was Updated: " + std::to_wstring(record->wasUpdated) };
                std::wstring missedshots{ L"Missed Shots: " + std::to_wstring(record->missedshots) };
                std::wstring lastworking{ L"Last Working Shot: " + std::to_wstring(record->lastworkingshot) };
                std::wstring PrevEye{ L"Previous Eye Angles (Y): " + std::to_wstring(record->PreviousEyeAngle) };
                std::wstring UpdateEyes{ L"Eye Angles On Sim Time Update: " + std::to_wstring(record->eyeAnglesOnUpdate) };
                std::wstring PrevHealth{ L"Previous Entity Health: " + std::to_wstring(record->prevhealth) };
                std::wstring PrevDesync{ L"Previous Desync Angle: " + std::to_wstring(record->PreviousDesyncAng) };
                std::wstring PrevVelocity{ L"Prev Velocity: " + std::to_wstring(record->prevVelocity) };
                std::wstring MultiExpan{ L"(DY) MultiPoint Expansion: " + std::to_wstring(record->multiExpan)};

                std::wstring invalid{ L"Invalid: " + std::to_wstring(record->invalid) };

                strings.push_back(Resolver);
                strings.push_back(simTime);
                strings.push_back(wasTargeted);
                strings.push_back(FiredUpon);
                strings.push_back(WasUpdated);
                strings.push_back(missedshots);
                strings.push_back(lastworking);
                strings.push_back(PrevEye);
                strings.push_back(UpdateEyes);
                strings.push_back(PrevHealth);
                strings.push_back(PrevDesync);
                strings.push_back(PrevVelocity);
                strings.push_back(MultiExpan);
                strings.push_back(invalid);

            }

            //std::wstring lolstr = L"YOMAMA ";
            //interfaces->surface->setTextPosition(100, 105);
            // interfaces->surface->printText(lolstr.c_str());

            Draw_Text(strings);


        }

    }
}

std::vector<std::wstring> Debug::formatRecord(Backtrack::Record record, Entity* entity, int index) {

    /*

    struct Record {
        Vector head;
        Vector origin;
        float simulationTime;
        matrix3x4 matrix[256];
        int PreviousAct;
        int prevhealth = 100;
        matrix3x4 prevResolvedMatrix[256];
        int missedshots = 0;
        bool wasTargeted = false;
    };
    std::wstring weightdelta{ L"Weight Delta Rate: " + std::to_wstring(AnimLayer->weightdeltarate) };

    */
    std::vector<std::wstring> strings;

    std::wstring playername{ L"PlayerName: " + std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(entity->getPlayerName(true)) };
    std::wstring bindex{ L"Index: " + std::to_wstring(index)};
    std::wstring simtime{ L"Simulation Time: " + std::to_wstring(record.simulationTime) };
    std::wstring prevhealth{ L"Previous Health: " + std::to_wstring(record.prevhealth) };
    std::wstring missedshots{ L"Missed Shots: " + std::to_wstring(record.missedshots) };
    std::wstring wasTargeted{ L"Was Targeted?: " + std::to_wstring(record.wasTargeted) };
    std::wstring wasUpdated{ L"Was Updated?: " + std::to_wstring(record.wasUpdated) };
    std::wstring resolvedAngles{ L"Resolved Angle: " + std::to_wstring(record.resolvedaddition) };

    strings = { playername, bindex, simtime, prevhealth, missedshots, wasTargeted, wasUpdated, resolvedAngles };
    return strings;



}

void Debug::BacktrackMonitor() noexcept {

    /*
    backtrack{
        bool enabled{false};
        ColorToggle color;
        int entityid{0};
        bool newesttick{false};
    }
    
    */
    if ((interfaces->engine->getMaxClients() < config->debug.backtrack.entityid) || (0 > config->debug.backtrack.entityid))
        return;

    auto entity = interfaces->entityList->getEntity(config->debug.backtrack.entityid);

    if (!entity && !(config->debug.backtrack.findactive))
        return;

    std::vector<std::vector<std::wstring>> strings_vecs;
    auto record = &Backtrack::records[entity->index()];

    if (!(record && record->size() && Backtrack::valid(record->front().simulationTime)) && !(config->debug.backtrack.findactive))
        return;

    if (config->debug.backtrack.newesttick) {
        strings_vecs.push_back(formatRecord(record->front(), entity, 0));
    }
    else if (config->debug.backtrack.findactive) {
        for (int i = 1; i <= interfaces->engine->getMaxClients(); i++) {
            record = &Backtrack::records[i];
            if (!(record && record->size() && Backtrack::valid(record->at(1).simulationTime)))
                continue;
            if (record->at(1).wasTargeted)
                strings_vecs.push_back(formatRecord(record->front(), interfaces->entityList->getEntity(i), 0));
                break;
            if (i == interfaces->engine->getMaxClients())
                return;
        }
    }
    else {
        int i = 0;
        for (auto rec : *record) {
            strings_vecs.push_back(formatRecord(rec, entity, i));
            i++;
        }
        
    }

    interfaces->surface->setTextColor(config->debug.backtrack.color.color);

    for (auto string_vec : strings_vecs) {
        Draw_Text(string_vec);
    }



}

void Debug::DrawDesyncInfo() {
	interfaces->surface->setTextColor(config->debug.desync_info.color);
	
	if (!localPlayer)
		return;

	std::wstring MaxAng{ L"Max Possible Desync Angle: " + std::to_wstring(localPlayer->getMaxDesyncAngle()) };
	std::wstring CurrYaw{ L"Current Player Yaw: "  };
	std::wstring CurrPitch{ L"Current Player Pitch: " };





}


void Debug::CustomHUD() {




    if (!localPlayer || !localPlayer->isAlive())
        return;

    /*
    static auto hudVar{ interfaces->cvar->findVar("cl_drawhud") };
    hudVar->onChangeCallbacks.size = 0;
    hudVar->setValue(0);
    */

    static auto hudVAr{ interfaces->cvar->findVar("cl_draw_only_deathnotices") };
    hudVAr->onChangeCallbacks.size = 0;
    hudVAr->setValue(1);



    static auto radarVAr{ interfaces->cvar->findVar("cl_drawhud_force_radar") };
    radarVAr->onChangeCallbacks.size = 0;
    radarVAr->setValue(1);


    static auto radarVAra{ interfaces->cvar->findVar("cl_hud_radar_scale") };
    radarVAra->onChangeCallbacks.size = 0;
    radarVAra->setValue(0.8f);


    static auto radarVArb{ interfaces->cvar->findVar("cl_radar_scale") };
    radarVArb->onChangeCallbacks.size = 0;
    radarVArb->setValue(1.0f);

    static auto radarVArc{ interfaces->cvar->findVar("hud_saytext_time") };
    radarVArc->onChangeCallbacks.size = 0;
    radarVArc->setValue(10);




    interfaces->surface->setTextFont(Surface::font);

    interfaces->surface->setTextColor(config->debug.CustomHUD.color);
    const auto [screenWidth, screenHeight] = interfaces->surface->getScreenSize();


    std::wstring health{ L"Health >  " + std::to_wstring(localPlayer->health()) };
    std::wstring armor{ L"Armor >  " + std::to_wstring(localPlayer->armor()) };


    auto activeWeapon = localPlayer->getActiveWeapon();
    std::wstring clip;
    if (!activeWeapon || !activeWeapon->clip()) {
        clip = L"";
    }
    else {
        clip = { L"Ammo >  " + std::to_wstring(activeWeapon->clip()) + L"/" + std::to_wstring(activeWeapon->reserveAmmoCount()) };
    }

    auto [text_size_w, text_size_h] = interfaces->surface->getTextSize(Surface::font, health.c_str());
    interfaces->surface->setTextPosition((((screenWidth / 8) + (screenWidth / 4)) / 2) - (text_size_w / 2), (screenHeight - 5 - text_size_h));
    interfaces->surface->printText(health.c_str());


    auto [text_size_wb, text_size_hb] = interfaces->surface->getTextSize(Surface::font, armor.c_str());
    text_size_w = text_size_wb;



    interfaces->surface->setTextPosition((((screenWidth * 2 / 8) + (screenWidth / 2)) / 2) - (text_size_w / 2), (screenHeight - 5 - text_size_hb));
    interfaces->surface->printText(armor.c_str());


    auto [text_size_wa, text_size_ha] = interfaces->surface->getTextSize(Surface::font, clip.c_str());
    text_size_w = text_size_wa;
    interfaces->surface->setTextPosition(((screenWidth + (screenWidth * 3 / 4)) / 2) - (text_size_w / 2), (screenHeight - 5 - text_size_ha));
    interfaces->surface->printText(clip.c_str());


}



#include "Hacks/Walkbot/nav_file.h"
#include "Hacks/Walkbot/nav_structs.h"
#include "Walkbot.h"


void Debug::run(){


	interfaces->surface->setTextFont(Surface::font);
	const auto [screenWidth, screenHeight] = interfaces->surface->getScreenSize();

    Screen.Width = screenWidth;
	Screen.Height = screenHeight;
    Screen.CurrPosW = 0;
    Screen.CurrPosH = 5;
    Screen.CurrColumnMax = 0;


    if (config->debug.CustomHUD.enabled)
        CustomHUD();

	if (config->debug.desync_info.enabled)
		DrawDesyncInfo();

    if (config->debug.animStateMon.enabled)
        AnimStateMonitor();

    if (config->debug.backtrack.color.enabled)
        BacktrackMonitor();



    if (config->debug.FPSBar) {
        coords start, end;
        std::wstring BASESTRING = L"AAAA";
        const auto [text_size_w, text_size_h] = interfaces->surface->getTextSize(Surface::font, BASESTRING.c_str());
        start.y = (text_size_h * 3);
        start.x = (Screen.Width - text_size_w) - 5;
        end.y = (text_size_h * 13);
        end.x = Screen.Width - 5;
        if (!config->misc.watermark.enabled) {
            start.y -= (text_size_h * 2);
            end.y -= (text_size_h * 2);
        }
        static auto frameRate = 1.0f;
        frameRate = 0.9f * frameRate + 0.1f * memory->globalVars->absoluteFrameTime;
        DrawGraphBox(start, end, 1.0f, 150.0f, 1.0f / frameRate, 0, L"FPS");

        float latency = 0.0f;
        if (auto networkChannel = interfaces->engine->getNetworkChannel(); networkChannel && networkChannel->getLatency(0) > 0.0f)
            latency = networkChannel->getLatency(0);

        end.x = start.x - 3;
        start.x = (start.x - 3) - text_size_w;
        DrawGraphBox(start, end, 1.0f, 200.0f, latency * 1000, 0, L"PING");
    }

    PrintLog();

    if (config->misc.walkbot) {
        interfaces->surface->setTextColor(config->debug.animStateMon.color);

        auto pmapname = reinterpret_cast<std::string*>(reinterpret_cast<uintptr_t>(memory->clientState) + 0x288);
        auto mapdir = reinterpret_cast<std::string*>(reinterpret_cast<uintptr_t>(memory->clientState) + 0x188);

        std::wstring BASESTRING = {L"Map name:" + std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(pmapname->c_str())};
        std::wstring BASESTRINGDIR = { L"Map Directory:" + std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(mapdir->c_str()) };
        std::wstring NAVFILE = { L"C:\\Program Files(x86)\\Steam\\steamapps\\common\\Counter - Strike Global Offensive\\csgo\\maps\\" + std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(pmapname->c_str()) + L".nav" };
        std::vector<std::wstring> text;

        text.push_back(BASESTRING);
        text.push_back(BASESTRINGDIR);
        text.push_back(NAVFILE);
        text.push_back(std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(Walkbot::NAVFILE).c_str());
        text.push_back(std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(Walkbot::exception).c_str());

        Draw_Text(text);


        



       

        
        if (Walkbot::NAVFILE != "") {
            //nav_mesh::vec3_t previous = { 0,0,0 };
            if (Walkbot::map_nav.getNavAreas()->empty()) {
                std::wstring pointstr = { L" Map Nav Area Empty " };
                std::vector<std::wstring> text = { pointstr };
                Draw_Text(text);
            }
            else {
                try {
                    if (localPlayer && !localPlayer->isDormant() && localPlayer->isAlive()) {
                        auto areaID = Walkbot::map_nav.getAreaID(localPlayer->origin());
                        std::wstring idstr = { L"Current NAV Area: " + std::to_wstring(areaID) };
                        std::wstring navInf = { L"nav_inf Map: " + std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(Walkbot::nav_inf.mapname.c_str())};
                        std::vector<std::wstring> text = { idstr, navInf };
                        Draw_Text(text);
                    }
                }
                catch (std::exception& e){

                    return;
                }

            }

        }
        
    

        nav_mesh::vec3_t playerOrigin;

        if (localPlayer && localPlayer->isAlive() && !localPlayer->isDormant()) {
            text = { L"X : " + std::to_wstring(playerOrigin.convertVector(localPlayer->origin()).x) + L"Y : " + std::to_wstring(playerOrigin.convertVector(localPlayer->origin()).y) + L"Z : " + std::to_wstring(playerOrigin.convertVector(localPlayer->origin()).z) };
            Draw_Text(text);
        }

        if (!(Walkbot::curr_path.empty())){
            for (int i = 0; i < Walkbot::curr_path.size(); i++) {
                nav_mesh::vec3_t point = Walkbot::curr_path.at(i);
                std::wstring pointstr = { L"X: " + std::to_wstring(point.x) + L" Y: " + std::to_wstring(point.x) + L" Z: " + std::to_wstring(point.z) };
                std::vector<std::wstring> text = { pointstr };
                Draw_Text(text);
            }
        }


    }

    if (config->misc.showDamagedone) {

        //auto font = interfaces->surface->CreateFont();
        //interfaces->surface->SetFontGlyphSet(font, "Anklepants-Regular", 20, 700, 0, 0, 0x200);


        if ( 3 < (memory->globalVars->realtime - Misc::lastUpdate)) {
            Misc::damagedone = 0;
        }
        if(Misc::damagedone > 0){
            std::wstring BASESTRING = { std::to_wstring(Misc::damagedone) };
            const auto [text_size_w, text_size_h] = interfaces->surface->getTextSize(Surface::font, BASESTRING.c_str());

            interfaces->surface->setTextColor(255,255,255, (1 - (((memory->globalVars->realtime - Misc::lastUpdate)) / 3.0f)) * 255);
            interfaces->surface->setTextPosition((Screen.Width/2 + 5), ((Screen.Height / 2) - (text_size_h / 2) - (text_size_h * 2)));
            interfaces->surface->printText(BASESTRING.c_str());
        }
    }


    if (config->debug.spoofconvar) {
        static auto lagcompVar{ interfaces->cvar->findVar("sv_showlagcompensation") };
        lagcompVar->onChangeCallbacks.size = 0;
        lagcompVar->setValue(config->debug.showlagcomp);

        static auto showimpactsVar{ interfaces->cvar->findVar("sv_showimpacts") };
        showimpactsVar->onChangeCallbacks.size = 0;
        showimpactsVar->setValue(config->debug.showimpacts);


        static auto drawotherVar{ interfaces->cvar->findVar("r_drawothermodels") };
        drawotherVar->onChangeCallbacks.size = 0;
        drawotherVar->setValue((config->debug.drawother ? 2 : 1));

        if (config->debug.grenadeTraject) {
            static auto grenadeVar{ interfaces->cvar->findVar("sv_grenade_trajectory") };
            grenadeVar->onChangeCallbacks.size = 0;
            grenadeVar->setValue(1);

            static auto grenadetimeVar{ interfaces->cvar->findVar("sv_grenade_trajectory_time") };
            grenadetimeVar->onChangeCallbacks.size = 0;
            grenadetimeVar->setValue(10);
        }


        static auto tracerVar{ interfaces->cvar->findVar("r_drawtracers_firstperson") };
        tracerVar->onChangeCallbacks.size = 0;
        tracerVar->setValue(config->debug.tracer);


        static auto bhitVar{ interfaces->cvar->findVar("sv_showbullethits") };
        bhitVar->onChangeCallbacks.size = 0;
        bhitVar->setValue(config->debug.bullethits);
    }
    // sv_grenade_trajectory 1 
    // sv_grenade_trajectory_time 10
    // r_drawtracers_firstperson 0
    // sv_showbullethits 1
    if (config->debug.indicators.enabled) {
        if (config->debug.indicators.baim) {

            if (!localPlayer || !localPlayer->isAlive())
                return;

            const auto activeWeapon = localPlayer->getActiveWeapon();
            if (!activeWeapon || !activeWeapon->clip())
                return;

            auto weaponIndex = getWeaponIndex(activeWeapon->itemDefinitionIndex2());
            if (!weaponIndex)
                return;

            if (config->aimbot[weaponIndex].baim && (GetAsyncKeyState(config->aimbot[weaponIndex].baimkey))){
                    std::wstring BASESTRING = L"Baim Enabled";
                    const auto [text_size_w, text_size_h] = interfaces->surface->getTextSize(Surface::font, BASESTRING.c_str());
                    interfaces->surface->setTextColor(config->debug.animStateMon.color);
                    interfaces->surface->setTextPosition((Screen.Width - text_size_w) - 5, ((Screen.Height / 2) -(text_size_h / 2) - text_size_h));
                    interfaces->surface->printText(BASESTRING.c_str());

            }
            if (config->debug.indicators.choke && (interfaces->engine->getNetworkChannel()->chokedPackets > 1)) {
                std::wstring BASESTRING = L"CHOKE";
                const auto [text_size_w, text_size_h] = interfaces->surface->getTextSize(Surface::font, BASESTRING.c_str());
                interfaces->surface->setTextColor(config->debug.animStateMon.color);
                interfaces->surface->setTextPosition((Screen.Width - text_size_w) - 5, ((Screen.Height / 2) - (text_size_h / 2) - text_size_h));
                interfaces->surface->printText(BASESTRING.c_str());
            }
        }


    }
    return;
}


/* INPUT */

void Debug::AnimStateModifier() noexcept
{
    if (!config->debug.AnimModifier)
        return;



    for (int i = 1; i <= interfaces->engine->getMaxClients(); i++) {



            auto entity = interfaces->entityList->getEntity(i);

            //if ((i != config->debug.entityid) && (config->debug.entityid != -1))
                //continue;

            if (!entity || entity->isDormant() || !entity->isAlive() || !entity->isPlayer())
                continue;

            if (entity == localPlayer.get())
                continue;

            auto AnimState = entity->getAnimstate();
            if (!AnimState)
                continue;

            if (config->debug.animstatedebug.manual) {
                entity->eyeAngles().x = config->debug.Pitch;
                //entity->eyeAngles().y = config->debug.Yaw;
                //AnimState->m_flGoalFeetYaw = config->debug.GoalFeetYaw;
            }

            if (config->debug.animstatedebug.resolver.enabled) {
                Resolver::BasicResolver(entity, config->debug.animstatedebug.resolver.missed_shots);
            }

            //AnimState = entity->getAnimstate();
           // if (!AnimState)
            //    continue;



            //AnimState->m_iLastClientSideAnimationUpdateFramecount -= 1;


            //Vector NewAnim{ config->debug.Pitch , config->debug.Yaw, 0.0f };
            //entity->setAbsAngle(NewAnim);

           // entity->UpdateState(AnimState, NewAnim);

            


        

    }


}