#include "Backtrack.h"
#include "Tickbase.h"

#include "../SDK/Entity.h"
#include "../SDK/UserCmd.h"


Tickbase::ForceCMD Tickbase::newCmd;

bool canShift(int ticks, bool shiftAnyways = false)
{
    if (!localPlayer || !localPlayer->isAlive() || ticks <= 0)
        return false;

    if (shiftAnyways)
        return true;

    if ((Tickbase::tick->ticksAllowedForProcessing - ticks) < 0)
        return false;

    if (localPlayer->nextAttack() > memory->globalVars->serverTime())
        return false;

    float nextAttack = (localPlayer->nextAttack() + (ticks * memory->globalVars->intervalPerTick));
    if (nextAttack >= memory->globalVars->serverTime())
        return false;

    auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon || !activeWeapon->clip() || activeWeapon->isGrenade())
        return false;

    if (activeWeapon->isKnife() || activeWeapon->isGrenade() 
        || activeWeapon->itemDefinitionIndex2() == WeaponId::Revolver
        || activeWeapon->itemDefinitionIndex2() == WeaponId::Awp
        || activeWeapon->itemDefinitionIndex2() == WeaponId::Ssg08
        || activeWeapon->itemDefinitionIndex2() == WeaponId::Taser
        || activeWeapon->itemDefinitionIndex2() == WeaponId::Revolver)
        return false;

    float shiftTime = (localPlayer->tickBase() - ticks) * memory->globalVars->intervalPerTick;

    if (shiftTime < activeWeapon->nextPrimaryAttack())
        return false;

    return true;
}

void recalculateTicks() noexcept
{
    Tickbase::tick->chokedPackets = std::clamp(Tickbase::tick->chokedPackets, 0, Tickbase::tick->maxUsercmdProcessticks);
    Tickbase::tick->ticksAllowedForProcessing = Tickbase::tick->maxUsercmdProcessticks - Tickbase::tick->chokedPackets;
    Tickbase::tick->ticksAllowedForProcessing = std::clamp(Tickbase::tick->ticksAllowedForProcessing, 0, Tickbase::tick->maxUsercmdProcessticks);
}

void Tickbase::shiftTicks(int ticks, UserCmd* cmd, bool shiftAnyways) noexcept //useful, for other funcs
{
    if (!localPlayer || !localPlayer->isAlive() || !config->misc.dt.enabled)
        return;

    if (!canShift(ticks, shiftAnyways)) {
        return;
    }


    tick->commandNumber = cmd->commandNumber;
    tick->tickbase = localPlayer->tickBase();
    tick->tickshift = ticks;

    if (newCmd.setcmd) {
        /* For DT Resolving*/
    }
}

void Tickbase::run(UserCmd* cmd) noexcept
{
    if (!config->misc.dt.enabled)
        return;

    if (!localPlayer || !localPlayer->isAlive() || localPlayer->isDormant() || (interfaces->engine->getNetworkChannel()->chokedPackets > 16) || ((localPlayer->getVelocity().length2D() > config->misc.fakelagspeed)) || config->misc.testshit.enabled)
        return;

    //if (localPlayer->getVelocity().length2D() > 60.0f)
    //    return;

    static void* oldNetwork = nullptr;
    if(auto network = interfaces->engine->getNetworkChannel(); network && oldNetwork != network)
    {
        oldNetwork = network;
        tick->ticksAllowedForProcessing = tick->maxUsercmdProcessticks;
        tick->chokedPackets = 0;
    }
    if (auto network = interfaces->engine->getNetworkChannel(); network && network->chokedPackets > tick->chokedPackets)
        tick->chokedPackets = network->chokedPackets;

    recalculateTicks();

    tick->ticks = cmd->tickCount;


    auto ticks = 0;

    switch (config->misc.dt.doubletapspeed - 1) {
    case 0: //Instant
        ticks = 16;
        break;
    case 1:
        ticks = 15;
        break;
    case 2: //Fast
        ticks = 14;
        break;
    case 3: //Accurate
        ticks = 13;
        break;
    case 4: //Accurate
        ticks = 12;
        break;
    }

    if ((cmd->buttons & UserCmd::IN_ATTACK) && ((config->misc.dt.enabled && GetAsyncKeyState(config->misc.dt.dtKey)) || (config->misc.dt.enabled && (config->misc.dt.dtKey == 0)))) { 
        shiftTicks(ticks, cmd); 
    }
    

    if ((tick->tickshift <= 0) && (tick->ticksAllowedForProcessing < (tick->maxUsercmdProcessticks - interfaces->engine->getNetworkChannel()->chokedPackets)) || ((interfaces->engine->getNetworkChannel()->chokedPackets <= (tick->maxUsercmdProcessticks - ticks))) && !config->misc.testshit.toggled && !cmd->buttons & UserCmd::IN_ATTACK)/*&& !config->antiAim.fakeDucking*/ /*&& ((interfaces->engine->getNetworkChannel()->chokedPackets <= (tick->maxUsercmdProcessticks - ticks)) || !config->misc.testshit.toggled) */
    {
        //newCmd.dtPrevTick = false;
        if (cmd->tickCount != INT_MAX) {
            cmd->tickCount = INT_MAX; //recharge
        }
        tick->chokedPackets--;
    }

    recalculateTicks();


}
