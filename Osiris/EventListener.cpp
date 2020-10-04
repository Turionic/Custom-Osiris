#include <cassert>

#include "EventListener.h"
#include "fnv.h"
#include "Hacks/Misc.h"
#include "Hacks/SkinChanger.h"
#include "Hacks/Visuals.h"
#include "Interfaces.h"
#include "Memory.h"
#include "Walkbot.h"
#include "Hacks/StreamProofESP.h"
#include "Debug.h"
#include "Grief.h"

EventListener::EventListener() noexcept
{
    assert(interfaces);

    interfaces->gameEventManager->addListener(this, "item_purchase");
    interfaces->gameEventManager->addListener(this, "round_start");
    interfaces->gameEventManager->addListener(this, "round_freeze_end");
    interfaces->gameEventManager->addListener(this, "player_hurt");

    interfaces->gameEventManager->addListener(this, "player_death");
    interfaces->gameEventManager->addListener(this, "bullet_impact");
    
    interfaces->gameEventManager->addListener(this, "hegrenade_detonate");
    interfaces->gameEventManager->addListener(this, "flashbang_detonate");

    interfaces->gameEventManager->addListener(this, "grenade_bounce");
    interfaces->gameEventManager->addListener(this, "molotov_detonate");
    if (const auto desc = memory->getEventDescriptor(interfaces->gameEventManager, "player_death", nullptr))
        std::swap(desc->listeners[0], desc->listeners[desc->listeners.size - 1]);
    else
        assert(false);
}

void EventListener::remove() noexcept
{
    assert(interfaces);

    interfaces->gameEventManager->removeListener(this);
}

void EventListener::fireGameEvent(GameEvent* event)
{
    switch (fnv::hashRuntime(event->getName())) {
    case fnv::hash("round_start"):
    case fnv::hash("item_purchase"):
    case fnv::hash("round_freeze_end"):
        Misc::purchaseList(event);
        Walkbot::SetupTime(event);
        break;
    case fnv::hash("player_death"):
        Grief::CalculateTeamDamage(event);
        SkinChanger::updateStatTrak(*event);
        SkinChanger::overrideHudIcon(*event);
        Misc::killMessage(*event);
        Misc::killSound(*event);
        break;
    case fnv::hash("bullet_impact"):
        Visuals::bulletBeams(event);
        StreamProofESP::bulletTracer(event);
        break;
    case fnv::hash("player_hurt"):
        Grief::CalculateTeamDamage(event);
        //StreamProofESP::bulletTracer(event); <---- Make Happen only on damage delt? As indicator?
        Misc::AttackIndicator(event);
        Misc::playHitSound(*event);
        Visuals::hitEffect(event);
        Visuals::hitMarker(event);
        Debug::Logger(event);
        break;
    case fnv::hash("hegrenade_detonate"):
    case fnv::hash("flashbang_detonate"):
        Visuals::grenadeBeams(event);
        //break;//lol fall through
    case fnv::hash("grenade_bounce"):
    case fnv::hash("molotov_detonate"):
        Visuals::BounceRing(event);
        break;
    }
}
