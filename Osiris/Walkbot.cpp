
#include "Walkbot.h"
#include "SDK/LocalPlayer.h"
#include "Memory.h"
#include "Interfaces.h"
#include "SDK/Entity.h"

#include "Hacks/Resolver.h"


#include "Hacks/Walkbot/nav_file.h"

#include <sstream>
#include <codecvt>
#include <locale>

nav_mesh::nav_file Walkbot::map_nav;
std::vector<nav_mesh::vec3_t> Walkbot::curr_path;
std::string Walkbot::NAVFILE = "";
std::string Walkbot::exception = "";
bool Walkbot::nav_fileExcept = false;
bool Walkbot::currentTarget = false;
//std::map<std::string, Walkbot::bombsite> Walkbot::bombsites;







/*
    struct nav_positions {
        int A;
        int B;
        int T_Spawn;
        int CT_Spawn;
        std::vector<int> mainPath;
    };

*/

Walkbot::nav_positions de_dust2{
        1402,
        1572,
        7186,
        1803,
        { 1572, 8075, 1287, 8985, 95, 1402, 4140, 4999, 4167, 1441, 7187 },
        "de_dust2"
};

Walkbot::nav_positions de_mirage{
    296,
    7,
    1,
    3,
    {73, 105, 7, 862, 59, 222, 797, 77, 1, 645, 82, 784, 98, 33, 135},
    "de_mirage"
};

Walkbot::nav_positions de_inferno{
    903,
    5,
    52,
    68,
    {20, 2633, 2626, 15, 71, 82, 208, 1668, 541, 89, 35},
    "de_inferno"
};

Walkbot::nav_positions de_overpass{
    7145,
    9106,
    272,
    12220,
    {12220, 7145, 9106, 272},
    "de_overpass"
};

Walkbot::nav_positions de_train{
    4122,
    1201,
    3973,
    3132,
    { 4122, 3973, 1201, 3132},
    "de_train"
};

std::unordered_map<std::string, Walkbot::nav_positions> Walkbot::maps = {
    {"de_dust2" , de_dust2},
    {"de_mirage", de_mirage},
    {"de_inferno", de_inferno},
    {"de_overpass", de_overpass},
    {"de_train", de_train}
};



Walkbot::nav_positions Walkbot::nav_inf{ 0,0,0,0,{0}, "de_invalid" };
std::vector<int> Walkbot::mainPath;



void Walkbot::InsurePath() {
    try {

        if (curr_path.empty()) {
            nav_mesh::vec3_t playerOrigin;
            if (mainPath.empty()) {
                mainPath = nav_inf.mainPath;
            }

            if (!mainPath.empty()) {
                curr_path = map_nav.find_path(playerOrigin.convertVector(localPlayer->origin()), mainPath.front());
            }

        }

        nav_fileExcept = true;
    }
    catch (std::exception& e) {
        nav_fileExcept = true;
        exception = e.what();
        return;
    }
    return;
}


bool Walkbot::InitLoad()
{
    auto mapname = reinterpret_cast<std::string*>(reinterpret_cast<uintptr_t>(memory->clientState) + 0x288);
    std::string str(mapname->c_str());
    NAVFILE = { "C:/Program Files (x86)/Steam/steamapps/common/Counter-Strike Global Offensive/csgo/maps/" + str + ".nav" };


    try {
        
        if(!nav_fileExcept || (nav_inf.mapname != str))
            map_nav.load((std::string_view)NAVFILE);

        if (map_nav.getNavAreas()->empty())
            throw std::exception("Nav Areas Empty after load?");

        if (curr_path.empty()) {
            nav_mesh::vec3_t playerOrigin;
            nav_inf = maps[str];
            if (mainPath.empty()) {
                mainPath = nav_inf.mainPath;
            }

            if(!mainPath.empty()) {
                curr_path = map_nav.find_path(playerOrigin.convertVector(localPlayer->origin()), mainPath.front());
            }

        }

        nav_fileExcept = true;
    } catch(std::exception& e){
            nav_fileExcept = true;
            exception = e.what();
            return false;
    }
    




    return true;
}

Vector calculateRelativeAngle(const Vector& source, const Vector& destination, const Vector& viewAngles) noexcept
{
    Vector delta = destination - source;
    Vector angles{ radiansToDegrees(atan2f(viewAngles.z, std::hypotf(delta.x, delta.y))) - viewAngles.x,
                   radiansToDegrees(atan2f(delta.y, delta.x)) - viewAngles.y };
    angles.normalize();
    return angles;
}


void Walkbot::HeadToPoint(UserCmd* cmd) {



    cmd->forwardmove = config->misc.walkbotspeed;

    try {
        if (map_nav.inArea(localPlayer->origin(), curr_path.front()) || !currentTarget) {


            curr_path.erase(curr_path.begin());
            if (!curr_path.empty()) {
                Vector angle = calculateRelativeAngle(localPlayer->getEyePosition(), curr_path.front().toVector(), cmd->viewangles);
                angle.y = std::clamp(angle.y, -50.0f, 50.0f);
                angle.x = std::clamp(angle.x, -50.0f, 50.0f);
                angle /= 1.5f;
                cmd->viewangles += angle;
                interfaces->engine->setViewAngles(cmd->viewangles);
                currentTarget = true;
            }

            //mainPath = { 1572, 8075, 1287, 8985, 95, 1402, 4140, 4999, 4167, 1441, 7187 };
            if (mainPath.size() == 1)
                mainPath = nav_inf.mainPath;

            if (map_nav.inArea(localPlayer->origin(), mainPath.front())) {
                nav_mesh::vec3_t playerOrigin;
                mainPath.erase(mainPath.begin());
                for (auto point : mainPath) {
                    try {
                        curr_path = map_nav.find_path(playerOrigin.convertVector(localPlayer->origin()), point);
                        break;
                    }
                    catch (std::exception& e) {
                        nav_fileExcept = true;
                        exception = e.what();
                        currentTarget = false;                      
                    }
                    if (!curr_path.empty())
                        return;
                }

            }

            return;


        }
        else if ((localPlayer->getVelocity().length2D() < 1.01f)) {
            if (!(cmd->tickCount % 10)) {
                cmd->buttons |= UserCmd::IN_JUMP;
            }
            else if (!(cmd->tickCount % 7) || !(cmd->tickCount % 8)){
                cmd->buttons |= UserCmd::IN_DUCK;
            }

            if (!curr_path.empty()) {
                nav_mesh::vec3_t playerOrigin;
                auto curr_backup = curr_path;
                try {
                    curr_path = map_nav.find_path(playerOrigin.convertVector(localPlayer->origin()), curr_path.back());
                } catch (std::exception& e){
                    curr_path = curr_backup;
                }
            }

            Vector angle = calculateRelativeAngle(localPlayer->getEyePosition(), curr_path.front().toVector(), cmd->viewangles);
            angle.y = std::clamp(angle.y, -50.0f, 50.0f);
            angle.x = std::clamp(angle.x, -50.0f, 50.0f);
            angle /= 1.5f;
            cmd->viewangles += angle;
            interfaces->engine->setViewAngles(cmd->viewangles);
            currentTarget = true;
        }
        else if(!curr_path.empty() && (localPlayer->velocity().length2D() < 10)){
            
            Vector angle = calculateRelativeAngle(localPlayer->getEyePosition(), curr_path.front().toVector(), cmd->viewangles);
            angle.y = std::clamp(angle.y, -50.0f, 50.0f);
            angle.x = std::clamp(angle.x, -50.0f, 50.0f);
            angle /= 1.5f;
            cmd->viewangles += angle;
            interfaces->engine->setViewAngles(cmd->viewangles);
            currentTarget = true;
        }

        //mainPath = { 1572, 8075, 1287, 8985, 95, 1402, 4140, 4999, 4167, 1441, 7187 };
        if(mainPath.size() == 1)
            mainPath = { 1572, 8075, 1287, 8985, 95, 1402, 4140, 4999, 4167, 1441, 7187 };

        if (map_nav.inArea(localPlayer->origin(), mainPath.front())) {
            mainPath.erase(mainPath.begin());
            nav_mesh::vec3_t playerOrigin;        
            for (auto point : mainPath) {
                try {
                    curr_path = map_nav.find_path(playerOrigin.convertVector(localPlayer->origin()), point);
                    break;
                }
                catch (std::exception& e) {
                    nav_fileExcept = true;
                    exception = e.what();
                    currentTarget = false;
                    return;
                }
            }
            //curr_path = map_nav.find_path(playerOrigin.convertVector(localPlayer->origin()), mainPath.front());
        }

    }
    catch (std::exception& e) {
        nav_fileExcept = true;
        exception = e.what();
        currentTarget = false;
        return;
    }
}


void Walkbot::TraversePath(UserCmd* cmd) {

    cmd->forwardmove = config->misc.walkbotspeed;

    bool clear = false;
    if (!curr_path.empty()) {

        /* If we don't currently have a target, or have reached a targeted nav point*/
        try {
            if (map_nav.inArea(localPlayer->origin(), curr_path.front()) || !currentTarget) {

                curr_path.erase(curr_path.begin());

                currentTarget = true;
                // if our current path is now empty, we try to set a new one
                if (curr_path.empty()) {
                    try {
                        curr_path = map_nav.find_path(nav_mesh::vec3_t::convertSDKVector(localPlayer->origin()), mainPath.front());
                    }
                    catch (std::exception& e) {
                        try {
                            curr_path.push_back(nav_mesh::vec3_t::convertSDKVector(map_nav.nearestArea(localPlayer->origin())));
                            clear = true;
                        }
                        catch (std::exception& e) {
                            currentTarget = false;
                        }
                        nav_fileExcept = true;
                        exception = e.what();                     
                    }
                }
            }
        }

        catch (std::exception& e) {
            nav_fileExcept = true;
            exception = e.what();
        }
    }

    try {
        if (!mainPath.empty()) {
            if (map_nav.inArea(localPlayer->origin(), mainPath.front())) {
                mainPath.erase(mainPath.begin());
            }
        }
    }
    catch (std::exception& e) {

    }

    if ((mainPath.size() == 1) || mainPath.empty()) {
        mainPath = nav_inf.mainPath;
    }



    if (localPlayer->velocity().length2D() < 1.0) { // not the best method by any shot, but it works
        if (curr_path.empty()) {
            try {
                curr_path.push_back(nav_mesh::vec3_t::convertSDKVector(map_nav.nearestArea(localPlayer->origin())));
            }
            catch (std::exception& e) {
                currentTarget = false;
            }
            clear = true;
        }

        if (!curr_path.empty()) {

            

            Vector angle = calculateRelativeAngle(localPlayer->getEyePosition(), curr_path.front().toVector(), cmd->viewangles);
            angle.y = std::clamp(angle.y, -50.0f, 50.0f); // clamp angles
            angle.x = std::clamp(angle.x, -50.0f, 50.0f); // clamp angles
            angle /= 1.5f;
            cmd->viewangles += angle;
            interfaces->engine->setViewAngles(cmd->viewangles);

            if (clear == true) {
                curr_path.clear();
            }
            else {
                if (localPlayer->PointToPoint(localPlayer->getEyePosition(), curr_path.front().toVector()) < .997) {
                    curr_path.clear();
                }
            }
        }


        
        Vector maxs = localPlayer->origin();
        Vector mins = localPlayer->origin();

        maxs += localPlayer->getCollideable()->obbMaxs() * 1.5;
        mins.x += localPlayer->getCollideable()->obbMins().x * 1.5;
        mins.y += localPlayer->getCollideable()->obbMins().y * 1.5;



        std::vector<Vector> TestPoints;

        TestPoints.push_back(Vector{ (maxs.x + mins.x) / 2, maxs.y, maxs.z });;
        TestPoints.push_back(Vector{ (maxs.x + mins.x) / 2, mins.y, maxs.z });;
        TestPoints.push_back(Vector{ maxs.x, (maxs.y + mins.y) / 2, maxs.z });
        TestPoints.push_back(Vector{ mins.x, (maxs.y + mins.y) / 2, maxs.z });

        TestPoints.push_back(Vector{ (maxs.x + mins.x) / 2, maxs.y, ((((mins.z + maxs.z)/2) / 2) / 2) });;
        TestPoints.push_back(Vector{ (maxs.x + mins.x) / 2, mins.y, ((((mins.z + maxs.z) / 2) / 2) / 2) });;
        TestPoints.push_back(Vector{ maxs.x, (maxs.y + mins.y) / 2, ((((mins.z + maxs.z) / 2) / 2) / 2) });
        TestPoints.push_back(Vector{ mins.x, (maxs.y + mins.y) / 2, ((((mins.z + maxs.z) / 2) / 2) / 2) });

        for (int i = 0; i < 2; i++) {
            for (int b = 0; b < 4; b++) {
                Vector Pos = (i == 0) ? localPlayer->getEyePosition() : localPlayer->origin();

                try {
                    if (localPlayer->PointToPoint(Pos, TestPoints.at(((i + 1) * b))) < .997 ) {
                        if (i == 1) {
                            cmd->buttons |= UserCmd::IN_JUMP;
                            return;
                        }
                        else {
                            cmd->buttons |= UserCmd::IN_DUCK;
                            return;
                        }
                    }
                }
                catch (std::exception& e) {
                    nav_fileExcept = true;
                    exception = e.what();
                    return;
                }

            }
        }



    }

    if (!curr_path.empty()) {
        Vector angle = calculateRelativeAngle(localPlayer->getEyePosition(), curr_path.front().toVector(), cmd->viewangles);

        if (/*((angle.y > 0.01f) || (angle.x > 0.01f))*/ true) { // If we are within 10 degrees of lookin at the next point already, fuck it
            angle.y = std::clamp(angle.y, -50.0f, 50.0f); // clamp angles
            angle.x = std::clamp(angle.x, -50.0f, 50.0f); // clamp angles
            cmd->viewangles += angle;
            interfaces->engine->setViewAngles(cmd->viewangles);
        }

        if (clear == true) {
            curr_path.clear();
        }
    }






}




                






void Walkbot::DeterminePointToTravel() {



}

#include "Hacks/Misc.h"
#include "SDK/GlobalVars.h"
#include "SDK/ConVar.h"
#include "SDK/GameEvent.h"
#include "fnv.h"

float Walkbot::freezeEnd = 0.0f;

void Walkbot::SetupTime(GameEvent* event) {

    if (!localPlayer)
        return;

    switch (fnv::hashRuntime(event->getName())) {
    case fnv::hash("round_start"):
        freezeEnd = memory->globalVars->realtime;
        break;
    case fnv::hash("round_freeze_end"):
        freezeEnd = 0.0f;
        break;
    }
}

void Walkbot::SetupBuy() {
    static const auto mp_buytime = interfaces->cvar->findVar("mp_buytime");

    if ((memory->globalVars->realtime < (freezeEnd + mp_buytime->getFloat())) && !localPlayer->gunGameImmunity()) {

        if ((localPlayer->team() == 2) && (localPlayer->getActiveWeapon()->isPistol()) && ((localPlayer->account() >= 800) || (localPlayer->gunGameImmunity() && (localPlayer->getActiveWeapon()->isPistol())))) {
            std::string cmd = "buy vest; buy vesthelm; buy gsg08; buy awp; buy ak47; buy p90; buy deagle";
            interfaces->engine->clientCmdUnrestricted(cmd.c_str());
        }
        else if ((localPlayer->team() == 3) && (localPlayer->getActiveWeapon()->isPistol()) && ((localPlayer->account() >= 800) || (localPlayer->gunGameImmunity() && (localPlayer->getActiveWeapon()->isPistol())))) {
            std::string cmd = "buy vest; buy defuser; buy scar20; buy awp; buy m4a1_silencer; buy p90; buy deagle";
            interfaces->engine->clientCmdUnrestricted(cmd.c_str());
        }
        else if ((localPlayer->armor() < 90) && (localPlayer->account() > 650) && !localPlayer->gunGameImmunity() && (freezeEnd != 0.0f)) {
            std::string cmd = "buy vest;";
            interfaces->engine->clientCmdUnrestricted(cmd.c_str());
        } else if (localPlayer->gunGameImmunity()){
            if (freezeEnd != 2.0) {
                if ((localPlayer->team() == 2)) {
                    std::string cmd = "buy ak47";
                    interfaces->engine->clientCmdUnrestricted(cmd.c_str());
                }
                else if ((localPlayer->team() == 3)) {
                    std::string cmd = "buy m4a1_silencer";
                    interfaces->engine->clientCmdUnrestricted(cmd.c_str());
                }
                freezeEnd = 2.0f;
            }
        }
        freezeEnd = 0.0f;
    }
}




void Walkbot::Run(UserCmd* cmd) {
    currentTarget = true;


    if (!localPlayer) {
        if (!curr_path.empty())
            curr_path.clear();

        currentTarget = false;
        return;
    }

    if (localPlayer->isDormant() || !localPlayer->isAlive()) {
        Resolver::TargetedEntity = localPlayer.get();
        if(!curr_path.empty())
            curr_path.clear();
        currentTarget = false;
        return;
    }

    auto mapname = reinterpret_cast<std::string*>(reinterpret_cast<uintptr_t>(memory->clientState) + 0x288);
    std::string str(mapname->c_str());

    if (!nav_fileExcept || (nav_inf.mapname != str) || NAVFILE == "") {
        InitLoad();
        currentTarget = false;
    }

    
    if (curr_path.empty()) {
        InsurePath();
        currentTarget = false;
    }
    
    if (localPlayer->gunGameImmunity()) {


        SetupBuy();
        /*
        if ((localPlayer->team() == 2) && (localPlayer->getActiveWeapon()->isPistol())) {
            std::string cmd = "buy ak47";
            interfaces->engine->clientCmdUnrestricted(cmd.c_str());
        }
        else if ((localPlayer->team() == 3) && (localPlayer->getActiveWeapon()->isPistol())) {
            std::string cmd = "buy scar20";
            interfaces->engine->clientCmdUnrestricted(cmd.c_str());
        }
        */
        curr_path.clear();
        Resolver::TargetedEntity = localPlayer.get();
        return;

    }
    else {
        SetupBuy();
    }

    if (cmd->buttons & UserCmd::IN_ATTACK) {
        cmd->forwardmove = 0;
        return;
    }
    //std::string clanTag = ".noclips";
    //memory->setClanTag(clanTag.c_str(), clanTag.c_str());

    if (Resolver::TargetedEntity && (Resolver::TargetedEntity != localPlayer.get()) && Resolver::TargetedEntity->isAlive() && !Resolver::TargetedEntity->isDormant() && Resolver::TargetedEntity->isVisible()) {
        //cmd->buttons = cmd->buttons | UserCmd::IN_BACK;
        //cmd->forwardmove = 0;
        //if (!curr_path.empty())
        //   curr_path.clear();
        currentTarget = false;
        return;
    }

    //if(localPlayer->velocity().length2D() < 1.0 && localPlayer-)
    Vector PrevAng = cmd->viewangles;
    TraversePath(cmd);

    cmd->viewangles.y = std::clamp(cmd->viewangles.y, PrevAng.y - 28.0f, PrevAng.y + 28.0f);
    cmd->viewangles.x = std::clamp(cmd->viewangles.x, PrevAng.x - 28.0f, PrevAng.x + 28.0f);
    cmd->viewangles.z = std::clamp(cmd->viewangles.z, PrevAng.z - 28.0f, PrevAng.z + 28.0f);
    interfaces->engine->setViewAngles(cmd->viewangles);

}