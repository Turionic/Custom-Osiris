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

    interfaces->surface->setDrawColor(config->debug.box.color[0]*255, config->debug.box.color[1]*255, config->debug.box.color[2]*255, 180);
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

    if (!config->debug.DamageLog.enabled)
        return;

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
        if (r_health > 0) {
            logEntry.color = { (int)config->debug.DamageLog.color[0] * 255,(int)config->debug.DamageLog.color[1] * 255, (int)config->debug.DamageLog.color[2] * 255 };
        }
        else {
            logEntry.color = { 255,0,0 };
        }
        std::wstring text = {L"Hit " + playerName + L" for " + std::to_wstring(health) + L"hp in the" + HitGroups[event->getInt("hitgroup")] + L"with a " + weapon + L" || " + std::to_wstring(r_health) + L"hp remaining"};
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

#include <iomanip>
template< typename T >
std::wstring to_hex_wstring(T val)
{
    std::wostringstream wstr;  // note the 'w'
    wstr << std::setfill(L'0') << std::setw(2*sizeof(T)) << std::hex << (uint_least8_t)val;
    return wstr.str();
}


template<class T>
std::wstring VarToWString(std::wstring varName, T &var, bool set = false ) {
    if (!set) {
        return std::wstring{ varName + L"(" + to_hex_wstring(&var) + L"): " + to_hex_wstring(var) + L" | " + std::to_wstring(var) };
    }

}

template<class T>
std::wstring VarToWStringCast(std::wstring varName, T& var, bool set = false) {
    if (!set) {
        return std::wstring{ varName + L"(" + to_hex_wstring(&var) + L"): " + to_hex_wstring(var) + L" | " + std::to_wstring((char)var) };
    }

}

template <class T>
static void EraseDeque(std::deque<T>& de, int offset) {
    for (int i = de.size(); i > offset; i--) {
        if (de.back().PrintToConsole) {
            for (std::wstring text : de.back().text) {
                std::wstring command = { L"echo \"[HARPOON] " + text + L" [HARPOON]\"" };
                const wchar_t* wcmd = command.c_str();
                size_t size = (wcslen(wcmd) + 1) * sizeof(wchar_t);
                char* cmd = new char[size];
                std::wcstombs(cmd, wcmd, size);
                interfaces->engine->clientCmdUnrestricted(cmd);
                delete cmd;
            }
            if (!de.back().PrintToScreen) {
                interfaces->engine->clientCmdUnrestricted("echo \"\"");
            }



        }
        de.pop_back();
    }
}

void Debug::PrintLog() {
    if (LOG_OUT.size() > 8) {
        EraseDeque(LOG_OUT, 8);
    }


    int i = 0;
    for (; i < LOG_OUT.size(); i++) {
        LogItem item = LOG_OUT.at(i);

        if (item.time_of_creation < 1.0f) {
            item.time_of_creation = memory->globalVars->realtime;
        }

        if ((item.time_of_creation + 10) < (memory->globalVars->realtime)) { EraseDeque(LOG_OUT, i); return; }

        if (!item.PrintToScreen)
            continue;

        //if (!config.misc.cout) {
        //    return;
        //}

        interfaces->surface->setTextFont(Surface::font);
        interfaces->surface->setTextColor(item.color[0], item.color[1], item.color[2], (1 - (((memory->globalVars->realtime - item.time_of_creation)) / 10.0f)) * 255);
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
            /*
                float m_flTimeSinceStartedMoving; //0x100
                float m_flTimeSinceStoppedMoving; //0x104
            */
            std::wstring timeSinceStartMoving{ L"Time Since Started Moving: " + std::to_wstring(AnimState->m_flTimeSinceStartedMoving) };
            std::wstring timeSinceStoppedMoving{ L"Time Since Started Moving: " + std::to_wstring(AnimState->m_flTimeSinceStoppedMoving) };
            std::wstring speedFrac{ L"stopToFullRunningFraction: " + std::to_wstring(AnimState->stopToFullRunningFraction)};




            std::vector<std::wstring> strings{ playername, clientblend, dispatchedsrc, dispatcheddst, blendin, cycle, prevcycle, weight, weightdelta, overlay, state, sequence, order, playbackrate, timeSinceStartMoving,timeSinceStoppedMoving, speedFrac};

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
                std::wstring simTime{ L"Simulation Time: " + std::to_wstring(record->ResolveInfo.prevSimTime) };
                std::wstring wasTargeted{ L"Was Targeted: " + std::to_wstring(record->wasTargeted) };
                std::wstring FiredUpon{ L"Fired Upon: " + std::to_wstring(record->FiredUpon) };
                std::wstring WasUpdated{ L"Was Updated: " + std::to_wstring(record->ResolveInfo.wasUpdated) };
                std::wstring missedshots{ L"Missed Shots: " + std::to_wstring(record->missedshots) };
                std::wstring totalshots{ L"Total Shots: " + std::to_wstring(record->totalshots) };
                std::wstring lastworking{ L"Last Working Shot: " + std::to_wstring(record->lastworkingshot) };
                std::wstring PrevEye{ L"Current Eye Angles (Y): " + std::to_wstring(record->ResolveInfo.CurrentSet.EyeAngles.y) };
                std::wstring UpdateEyes{ L"Eye Angles On Sim Time Update: " + std::to_wstring(record->ResolveInfo.Original.EyeAngles.y) };
                std::wstring PrevHealth{ L"Previous Entity Health: " + std::to_wstring(record->prevhealth) };
                std::wstring PrevDesync{ L"Current Desync Angle: " + std::to_wstring(record->ResolveInfo.CurrentSet.LBYAngle) };
                std::wstring PrevVelocity{ L"Prev Velocity: " + std::to_wstring(record->prevVelocity) };
                std::wstring MultiExpan{ L"(DY) MultiPoint Expansion: " + std::to_wstring(record->multiExpan)};

                std::wstring wasLBY{ L"Was LBY Updated: " + std::to_wstring(record->lbyUpdated) };
                std::wstring nextLBY{ L"Next Predicted LBY Update " + std::to_wstring(record->lbyNextUpdate) };

                std::wstring invalid{ L"Invalid: " + std::to_wstring(record->invalid) };


                strings.push_back(Resolver);
                strings.push_back(simTime);
                strings.push_back(wasTargeted);
                strings.push_back(FiredUpon);
                strings.push_back(WasUpdated);
                strings.push_back(missedshots);
                strings.push_back(totalshots);
                strings.push_back(lastworking);
                strings.push_back(PrevEye);
                strings.push_back(UpdateEyes);
                strings.push_back(PrevHealth);
                strings.push_back(PrevDesync);
                strings.push_back(PrevVelocity);
                strings.push_back(MultiExpan);
                strings.push_back(wasLBY);
                strings.push_back(nextLBY);
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
    std::wstring onshot{ L"Onshot: " + std::to_wstring(record.onshot) };
    std::wstring lbyupdate{ L"LBY Update: " + std::to_wstring(record.lbyUpdated) };

    strings = { playername, bindex, simtime, onshot, lbyupdate};
    return strings;



}



static std::vector<std::wstring> byteArraytoText(std::wstring variableName, unsigned char* array, int sizeofarray) {
    std::vector<std::wstring> text;
    text.push_back(std::wstring{ variableName + L":" });
    unsigned char typeinit = (unsigned char)0;
    for (int i = 0; i < sizeofarray; i++) {
        typeinit = (unsigned char)*(array + (sizeof(unsigned char)*i));
        std::wostringstream IntRep;
        std::wostringstream inc;
        IntRep << std::setfill(L'0') << std::setw(3) << (uint_least8_t)typeinit;
        inc << std::setfill(L'0') << std::setw(3) << (i);
        text.push_back(std::wstring{ L"   Byte [" + inc.str() + L"]:  (" + to_hex_wstring(typeinit) + L") : " + IntRep.str() +  L" : " + std::to_wstring(static_cast<char>(typeinit))});
    }
    return text;

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

    AnimState* animstate = localPlayer->getAnimstate();

    Vector CurrEye = localPlayer->eyeAngles();

    std::wstring Desync{ L"Anti-Aim/Desync Info:" };
	std::wstring MaxAng{ L"Max Possible Desync Angle: " + std::to_wstring(localPlayer->getMaxDesyncAngle()) };
    std::wstring CurrYaw{ L"(AnimState LBY) Yaw: " + std::to_wstring(animstate->m_flCurrentFeetYaw) };
    std::wstring lbyYaw{ L"(LocalPlayerAA LBY LBY) Yaw: " + std::to_wstring(AntiAim::LocalPlayerAA.lby.y) };
    std::wstring reyeYaw{ L"(LocalPlayerAA Real Eye) Yaw: " + std::to_wstring(AntiAim::LocalPlayerAA.real.y) };
    std::wstring feyeYaw{ L"(LocalPlayerAA Fake Eye) Yaw: " + std::to_wstring(AntiAim::LocalPlayerAA.fake.y) };
    std::wstring pblbyrYaw{ L"(Pre-Break LBY Real) Yaw: " + std::to_wstring(AntiAim::LocalPlayerAA.PreBreakAngle) };
    std::wstring pblbyfYaw{ L"(Pre-Break LBY Fake) Yaw: " + std::to_wstring(AntiAim::LocalPlayerAA.PreBreakAngleFake) };
    std::wstring CurrPitch{ L"(Real) Pitch: " + std::to_wstring(CurrEye.x) };
    std::wstring lastlby{ L"Last LBY: " + std::to_wstring(AntiAim::LocalPlayerAA.lastlbyval) };
    std::wstring nextlby{ L"Next LBY: " + std::to_wstring(AntiAim::LocalPlayerAA.lbyNextUpdate) };
    std::wstring netset{ L"netset: " + std::to_wstring(AntiAim::LocalPlayerAA.netset) };
    std::wstring lastout{ L"lastOutgoing: " + std::to_wstring(AntiAim::LocalPlayerAA.lastOutgoing) };
    std::wstring OutSequenceNr = { L"OutSequenceNr: " + std::to_wstring(AntiAim::LocalPlayerAA.netchan.OutSequenceNr) };
    //std::wstring OutSequenceNr = VarToWString(std::wstring{ L"OutSequenceNr:" }, AntiAim::LocalPlayerAA.netchan.OutSequenceNr);
    std::vector<std::wstring> text = {Desync, std::wstring{L""}, MaxAng,CurrYaw,lbyYaw,reyeYaw,feyeYaw,pblbyrYaw,pblbyfYaw,CurrPitch,lastlby,nextlby,netset,lastout,OutSequenceNr,std::wstring{L""}};

    Draw_Text(text);


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


/*

    int OutSequenceNr;
    int InSequenceNr;
    int OutSequenceNrAck;
    int OutReliableState;
    int InReliableState;
    int chokedPackets;


*/

/*


    //std::byte pad[24];

std::byte pad[20];
unsigned char m_bProcessingMessages;
unsigned char m_bShouldDelete;
char pad_0x0016[0x2];

int OutSequenceNr;
int InSequenceNr;
int OutSequenceNrAck;
int OutReliableState;
int InReliableState;
int chokedPackets;
//bf_write m_StreamReliable; //0x0030 
std::byte  m_StreamReliable[24];
//CUtlMemory m_ReliableDataBuffer; //0x0048 
std::byte m_ReliableDataBuffer[12];
//bf_write m_StreamUnreliable; //0x0054 
std::byte  m_StreamUnreliable[24];
//CUtlMemory m_UnreliableDataBuffer; //0x006C 
std::byte  m_UnreliableDataBuffer[12];
//bf_write m_StreamVoice; //0x0078 
std::byte m_StreamVoice[24];
//CUtlMemory m_VoiceDataBuffer; //0x0090 
std::byte m_VoiceDataBuffer[12];
__int32 m_Socket; //0x009C 
__int32 m_StreamSocket; //0x00A0 
__int32 m_MaxReliablePayloadSize; //0x00A4 
char pad_0x00A8[0x4]; //0x00A8
//netadr_t remote_address; //0x00AC 
std::byte remote_address[12];
float last_received; //0x00B8 
char pad_0x00BC[0x4]; //0x00BC
float connect_time; //0x00C0 
char pad_0x00C4[0x4]; //0x00C4
__int32 m_Rate; //0x00C8 
float m_fClearTime; //0x00CC 
char pad_0x00D0[0x8]; //0x00D0
//CUtlVector m_WaitingList[0]; //0x00D8 
std::byte  m_WaitingList0[12];
//CUtlVector m_WaitingList[1]; //0x00EC 
std::byte m_WaitingList1[12];
char pad_0x0100[0x4120]; //0x0100
__int32 m_PacketDrop; //0x4220 
char m_Name[32]; //0x4224 
__int32 m_ChallengeNr; //0x4244 
float m_Timeout; //0x4248 
//INetChannelHandler* m_MessageHandler; //0x424C 
std::byte m_MessageHandler[4];
//CUtlVector m_NetMessages; //0x4250 
std::byte m_NetMessages[12];
__int32 m_pDemoRecorder; //0x4264 
__int32 m_nQueuedPackets; //0x4268 
float m_flInterpolationAmount; //0x426C 
float m_flRemoteFrameTime; //0x4270 
float m_flRemoteFrameTimeStdDeviation; //0x4274 
float m_nMaxRoutablePayloadSize; //0x4278 
__int32 m_nSplitPacketSequence; //0x427C 
char pad_0x4280[0x14]; //0x4280

*/


void Debug::NetworkChannelDebug(NetworkChannel* netchannel) {

   
    if (!netchannel)
        return;
    interfaces->surface->setTextColor(config->debug.networkchannel.color);
    std::wstring netchann = { L"Network Channel: " };
    std::wstring chockedPackets = {L"chokedPackets: " + std::to_wstring(netchannel->chokedPackets)};
    std::wstring InReliableState = { L"InReliableState: " + std::to_wstring(netchannel->InReliableState) };
    std::wstring OutReliableState = { L"OutReliableState: " + std::to_wstring(netchannel->OutReliableState) };
    std::wstring InSequenceNr = { L"InSequenceNr: " + std::to_wstring(netchannel->InSequenceNr) };
    std::wstring OutSequenceNr = { L"OutSequenceNr: " + std::to_wstring(netchannel->OutSequenceNr) };
    std::wstring OutSequenceNrAck = { L"OutSequenceNrAck: " + std::to_wstring(netchannel->OutSequenceNrAck) };
    std::vector<std::wstring> text = {netchann, std::wstring{L""}, chockedPackets, InReliableState, OutReliableState, InReliableState, InSequenceNr, OutSequenceNr,OutSequenceNrAck};
    //VarToWString(std::wstring{ L"OutSequenceNr:" }, AntiAim::LocalPlayerAA.netchan.OutSequenceNr);
    // , std::wstring{L""}
    /*
    
    text.push_back(VarToWStringCast(std::wstring{L"netchannel->m_bProcessingMessages"}, netchannel->m_bProcessingMessages));
    text.push_back(VarToWStringCast(std::wstring{L"netchannel->m_bShouldDelete" },netchannel->m_bShouldDelete));
    text.push_back(VarToWString(std::wstring{L"netchannel->last_received" }, netchannel->last_received));
    text.push_back(VarToWString(std::wstring{L"netchannel->m_ChallengeNr" }, netchannel->m_ChallengeNr));
    text.push_back(VarToWString(std::wstring{L"netchannel->m_fClearTime" }, netchannel->m_fClearTime));
    text.push_back(VarToWString(std::wstring{L"netchannel->m_flInterpolationAmount" }, netchannel->m_flInterpolationAmount));
    text.push_back(VarToWString(std::wstring{L"netchannel->m_flRemoteFrameTime" }, netchannel->m_flRemoteFrameTime));
    text.push_back(VarToWString(std::wstring{L"netchannel->m_flRemoteFrameTimeStdDeviation" }, netchannel->m_flRemoteFrameTimeStdDeviation));
    text.push_back(VarToWString(std::wstring{L"netchannel->m_MaxReliablePayloadSize" }, netchannel->m_MaxReliablePayloadSize));
    //text.push_back(VarToWString(std::wstring{L"netchannel->m_MessageHandler" }, netchannel->m_MessageHandler));
    text.push_back(VarToWStringCast(std::wstring{L"netchannel->m_Name" }, netchannel->m_Name));
    //text.push_back(VarToWString(std::wstring{L"netchannel->m_NetMessages" }, netchannel->m_NetMessages));
    text.push_back(VarToWString(std::wstring{L"netchannel->m_nMaxRoutablePayloadSize" }, netchannel->m_nMaxRoutablePayloadSize));

    text.push_back(VarToWString(std::wstring{L"netchannel->m_nQueuedPackets" }, netchannel->m_nQueuedPackets));
    text.push_back(VarToWString(std::wstring{L"netchannel->m_nSplitPacketSequence" }, netchannel->m_nSplitPacketSequence));
    text.push_back(VarToWString(std::wstring{L"netchannel->m_PacketDrop" }, netchannel->m_PacketDrop));
    text.push_back(VarToWString(std::wstring{L"netchannel->m_pDemoRecorder" }, netchannel->m_pDemoRecorder));
    text.push_back(VarToWString(std::wstring{L"netchannel->m_Rate" }, netchannel->m_Rate));
    text.push_back(VarToWString(std::wstring{L"netchannel->m_Socket;" }, netchannel->m_Socket));
    text.push_back(VarToWString(std::wstring{L"netchannel->m_Timeout" }, netchannel->m_Timeout));
    */
    Draw_Text(text);
    //text = byteArraytoText(std::wstring{L"m_ReliableDataBuffer"}, (unsigned char*)netchannel->m_ReliableDataBuffer, 12);
    //Draw_Text(text);
    //text = byteArraytoText(std::wstring{ L"m_StreamVoice" }, (unsigned char*)netchannel->m_StreamVoice, 24);
    //Draw_Text(text);
}

/*
class ClientState
{
public:
    void ForceFullUpdate()
    {
        deltaTick = -1;
    }
    std::byte		pad0[0x9C];
    NetworkChannel* netChannel;
    int				challengeNr;
    std::byte		pad1[0x64];
    int				signonState;
    std::byte		pad2[0x8];
    float			nextCmdTime;
    int				serverCount;
    int				currentSequence;
    std::byte		pad3[0x54];
    int				deltaTick;
    bool			paused;
    std::byte		pad4[0x7];
    int				viewEntity;
    int				playerSlot;
    char			levelName[260];
    char			levelNameShort[80];
    char			groupName[80];
    std::byte		pad5[0x5C];
    int				maxClients;
    std::byte		pad6[0x4984];
    float			lastServerTickTime;
    bool			InSimulation;
    std::byte		pad7[0x3];
    int				oldTickcount;
    float			tickRemainder;
    float			frameTime;
    int				lastOutgoingCommand;
    int				chokedCommands;
    int				lastCommandAck;
    int				commandAck;
    int				soundSequence;
    std::byte		pad8[0x50];
    Vector			angViewPoint;
    std::byte		pad9[0xD0];
    EventInfo* pEvents;
};


*/


void Debug::ClientstateDebug() {

    ClientState* clientState = memory->clientState;
    if (!clientState)
        return;
    interfaces->surface->setTextColor(config->debug.clientstate.color);
    std::wstring clientStat = { L"Client State: " };
    std::wstring challengeNr = { L"challengeNr: " + std::to_wstring(clientState->challengeNr) };
    std::wstring signonState = { L"signonState: " + std::to_wstring(clientState->signonState) };
    std::wstring nextCmdTime = { L"nextCmdTime: " + std::to_wstring(clientState->nextCmdTime) };
    std::wstring serverCount = { L"serverCount: " + std::to_wstring(clientState->serverCount) };
    std::wstring currentSequence = { L"currentSequence: " + std::to_wstring(clientState->currentSequence) };
    std::wstring deltaTick = { L"deltaTick: " + std::to_wstring(clientState->deltaTick) };
    std::wstring paused = { L"paused: " + std::to_wstring(clientState->paused) };
    std::wstring viewEntity = { L"viewEntity: " + std::to_wstring(clientState->viewEntity) };
    std::wstring maxClients = { L"maxClients: " + std::to_wstring(clientState->maxClients) };
    std::wstring lastServerTickTime = { L"lastServerTickTime: " + std::to_wstring(clientState->lastServerTickTime) };
    std::wstring InSimulation = { L"InSimulation: " + std::to_wstring(clientState->InSimulation) };
    std::wstring oldTickcount = { L"oldTickcount: " + std::to_wstring(clientState->oldTickcount) };
    std::wstring tickRemainder = { L"tickRemainder: " + std::to_wstring(clientState->tickRemainder) };
    std::wstring frameTime = { L"frameTime: " + std::to_wstring(clientState->frameTime) };
    std::wstring lastOutgoingCommand = { L"lastOutgoingCommand: " + std::to_wstring(clientState->lastOutgoingCommand) };
    std::wstring chokedCommands = { L"chokedCommands: " + std::to_wstring(clientState->chokedCommands) };
    std::wstring lastCommandAck = { L"lastCommandAck: " + std::to_wstring(clientState->lastCommandAck) };
    std::wstring commandAck = { L"commandAck: " + std::to_wstring(clientState->commandAck) };
    std::wstring soundSequence = { L"soundSequence: " + std::to_wstring(clientState->soundSequence) };


    std::vector<std::wstring> text = { clientStat , std::wstring{L""}, challengeNr , signonState , nextCmdTime , serverCount, currentSequence, deltaTick, paused,viewEntity, maxClients, lastServerTickTime, InSimulation,oldTickcount, tickRemainder,frameTime,lastOutgoingCommand,
                                       chokedCommands, lastCommandAck, commandAck, soundSequence, std::wstring{L""} };
    Draw_Text(text);
 
}
/*
struct GlobalVars {
    const float realtime;
    const int framecount;
    const float absoluteFrameTime;
    const std::byte pad[4];
    float currenttime;
    float frametime;
    const int maxClients;
    const int tickCount;
    const float intervalPerTick;

    float GetIPT() noexcept;
    float serverTime(UserCmd* = nullptr) noexcept;
};


*/

void Debug::GlobalVarDebug() {

    GlobalVars* globalvar = memory->globalVars;
    if (!globalvar)
        return;
    interfaces->surface->setTextColor(config->debug.globalvars.color);
    std::wstring globvar = { L"Global Vars: " };
    std::wstring realtime = std::wstring{ L"realtime: " + std::to_wstring(globalvar->realtime)};
    std::wstring framecount = std::wstring{ L"framecount: " + std::to_wstring(globalvar->framecount) };
    std::wstring absoluteFrameTime = std::wstring{ L"absoluteFrameTime: " + std::to_wstring(globalvar->absoluteFrameTime) };
    std::wstring currenttime = std::wstring{ L"currenttime: " + std::to_wstring(globalvar->currenttime) };
    std::wstring frametime = std::wstring{ L"frametime: " + std::to_wstring(globalvar->frametime) };
    std::wstring maxClients = std::wstring{ L"maxClients: " + std::to_wstring(globalvar->maxClients) };
    std::wstring tickCount = std::wstring{ L"tickCount: " + std::to_wstring(globalvar->tickCount) };
    std::wstring GetIPT = std::wstring{ L"GetIPT(): " + std::to_wstring(globalvar->GetIPT()) };
    std::wstring serverTime = std::wstring{ L"serverTime(): " + std::to_wstring(globalvar->serverTime()) };

    std::vector<std::wstring> text = { globvar,  std::wstring{L""} ,realtime, framecount, absoluteFrameTime,currenttime,frametime,maxClients,tickCount, GetIPT, serverTime, std::wstring{L""} };

    Draw_Text(text);


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

    
    if (config->debug.airstucktoggle) {
        std::wstring BASESTRING = L"AIRSTUCK";
        const auto [text_size_w, text_size_h] = interfaces->surface->getTextSize(Surface::font, BASESTRING.c_str());
        interfaces->surface->setTextColor(config->debug.animStateMon.color);
        interfaces->surface->setTextPosition((Screen.Width - text_size_w) - 5, ((Screen.Height / 2) - (text_size_h / 2) - text_size_h));
        interfaces->surface->printText(BASESTRING.c_str());
    }

    if (config->debug.CustomHUD.enabled)
        CustomHUD();

	if (config->debug.desync_info.enabled)
		DrawDesyncInfo();

    if (config->debug.animStateMon.enabled)
        AnimStateMonitor();

    if (config->debug.backtrack.color.enabled)
        BacktrackMonitor();

    if (config->debug.networkchannel.enabled)
        NetworkChannelDebug();

    if (config->debug.clientstate.enabled)
        ClientstateDebug();

    if (config->debug.globalvars.enabled)
        GlobalVarDebug();

    if (config->debug.graph.enabled) {
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
        if (config->debug.graph.FPSBar) {
            static auto frameRate = 1.0f;
            frameRate = 0.9f * frameRate + 0.1f * memory->globalVars->absoluteFrameTime;
            DrawGraphBox(start, end, 1.0f, 150.0f, 1.0f / frameRate, 0, L"FPS");
            end.x = start.x - 3;
            start.x = (start.x - 3) - text_size_w;
        }

        if (config->debug.graph.Ping) {
            float latency = 0.0f;
            if (auto networkChannel = interfaces->engine->getNetworkChannel(); networkChannel && networkChannel->getLatency(0) > 0.0f)
                latency = networkChannel->getLatency(0);

            DrawGraphBox(start, end, 1.0f, 200.0f, latency * 1000, 0, L"PING");
            end.x = start.x - 3;
            start.x = (start.x - 3) - text_size_w;

        }
        if (config->misc.chokedPackets && config->debug.graph.FakeLag) {
            DrawGraphBox(start, end, 0, config->misc.chokedPackets, interfaces->engine->getNetworkChannel()->chokedPackets, 0, L"LAG");
            end.x = start.x - 3;
            start.x = (start.x - 3) - text_size_w;
        }

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



        }
        if ((config->debug.indicators.choke && (interfaces->engine->getNetworkChannel()->chokedPackets > 1)) || config->debug.airstucktoggle) {
            std::wstring BASESTRING;
            if (config->debug.airstucktoggle) {
                BASESTRING = L"AIRSTUCK";
            }
            else {
                BASESTRING = L"CHOKE";
            }
            const auto [text_size_w, text_size_h] = interfaces->surface->getTextSize(Surface::font, BASESTRING.c_str());
            interfaces->surface->setTextColor(config->debug.animStateMon.color);
            interfaces->surface->setTextPosition((Screen.Width - text_size_w) - 5, ((Screen.Height / 2) - (text_size_h / 2) - text_size_h));
            interfaces->surface->printText(BASESTRING.c_str());
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
                entity->eyeAngles().x += config->debug.Pitch;
                Vector ABS = entity->getAbsAngle();
                ABS.y += config->debug.ABS;
                entity->setAbsAngle(ABS);
                entity->eyeAngles().y += config->debug.Yaw;
                AnimState->m_flGoalFeetYaw += config->debug.GoalFeetYaw;
                entity->UpdateState(AnimState, entity->eyeAngles());
            }

            if (config->debug.animstatedebug.resolver.enabled) {
                //Resolver::BasicResolver(entity, config->debug.animstatedebug.resolver.missed_shots);
                Resolver::AnimStateResolver(entity);
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