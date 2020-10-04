#include "Animations.h"

#include "../Memory.h"
#include "../MemAlloc.h"
#include "../Interfaces.h"
#include "AntiAim.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/Cvar.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/Entity.h"
#include "../SDK/UserCmd.h"
#include "../SDK/ConVar.h"
#include "../SDK/Input.h"

Animations::Datas Animations::data;

void Animations::update(UserCmd* cmd, bool& sendPacket) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;
    data.viewangles = cmd->viewangles;
    data.sendPacket = sendPacket;
}

void Animations::fake(UserCmd* cmd) noexcept
{
    static AnimState* fakeanimstate = nullptr;
    static bool updatefakeanim = true;
    static bool initfakeanim = true;
    static float spawnTime = 0.f;

    if (!interfaces->engine->isInGame())
    {
        updatefakeanim = true;
        initfakeanim = true;
        spawnTime = 0.f;
        fakeanimstate = nullptr;
    }

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (spawnTime != localPlayer.get()->spawnTime() || updatefakeanim)
    {
        spawnTime = localPlayer.get()->spawnTime();
        initfakeanim = false;
        updatefakeanim = false;
    }

    if (!initfakeanim)
    {
        fakeanimstate = static_cast<AnimState*>(memory->memalloc->Alloc(sizeof(AnimState)));

        if (fakeanimstate != nullptr)
            localPlayer.get()->CreateState(fakeanimstate);

        initfakeanim = true;
    }
    if (((cmd->tickCount != INT_MAX) || config->antiAim.airstuckonLBY) && ((data.sendPacket && (!AntiAim::lbyNextUpdatedPrevtick || localPlayer->velocity().length2D() > 1.125f)) || ((!AntiAim::lbyNextUpdatedPrevtick && AntiAim::lbyNextUpdated) || (AntiAim::lbyNextUpdatedPrevtick && AntiAim::lbyNextUpdated)) && localPlayer->velocity().length2D() < 1.125f))
    {
        std::array<AnimationLayer, 15> networked_layers;

        std::memcpy(&networked_layers, localPlayer.get()->animOverlays(), sizeof(AnimationLayer) * localPlayer->getAnimationLayerCount());
        auto backup_abs_angles = localPlayer.get()->getAbsAngle();
        auto backup_poses = localPlayer.get()->pose_parameters();

        *(uint32_t*)((uintptr_t)localPlayer.get() + 0xA68) = 0;

        localPlayer.get()->UpdateState(fakeanimstate, data.viewangles);
        localPlayer.get()->InvalidateBoneCache();
        memory->setAbsAngle(localPlayer.get(), Vector{ 0, fakeanimstate->m_flGoalFeetYaw, 0 });
        std::memcpy(localPlayer.get()->animOverlays(), &networked_layers, sizeof(AnimationLayer) * localPlayer->getAnimationLayerCount());

        data.gotMatrix = localPlayer.get()->setupBones(Animations::data.fakematrix, 256, 0x7FF00, memory->globalVars->currenttime);
        const auto origin = localPlayer.get()->getRenderOrigin();
        if (data.gotMatrix)
        {
            for (auto& i : data.fakematrix)
            {
                i[0][3] -= origin.x;
                i[1][3] -= origin.y;
                i[2][3] -= origin.z;
            }
        }
        std::memcpy(localPlayer.get()->animOverlays(), &networked_layers, sizeof(AnimationLayer) * localPlayer->getAnimationLayerCount());
        localPlayer.get()->pose_parameters() = backup_poses;
        memory->setAbsAngle(localPlayer.get(), Vector{ 0,backup_abs_angles.y,0 });
    }
}

void Animations::real() noexcept
{
    static auto jigglebones = interfaces->cvar->findVar("r_jiggle_bones");
    jigglebones->setValue(0);

    if (!localPlayer)
        return;

    if (!localPlayer->isAlive())
    {
        localPlayer.get()->ClientSideAnimation() = true;
        return;
    }

    static auto backup_poses = localPlayer.get()->pose_parameters();
    static auto backup_abs = localPlayer.get()->getAnimstate()->m_flGoalFeetYaw;

    if (!memory->input->isCameraInThirdPerson) {
        localPlayer.get()->ClientSideAnimation() = true;
        localPlayer.get()->UpdateClientSideAnimation();
        localPlayer.get()->ClientSideAnimation() = false;
        return;
    }
    
    static std::array<AnimationLayer, 15> networked_layers;

    while (localPlayer.get()->getAnimstate()->m_iLastClientSideAnimationUpdateFramecount == memory->globalVars->framecount)
        localPlayer.get()->getAnimstate()->m_iLastClientSideAnimationUpdateFramecount -= 1;

    static int old_tick = 0;
    if (old_tick != memory->globalVars->tickCount)
    {
        old_tick = memory->globalVars->tickCount;
        std::memcpy(&networked_layers, localPlayer.get()->animOverlays(), sizeof(AnimationLayer) * localPlayer->getAnimationLayerCount());
        localPlayer.get()->ClientSideAnimation() = true;
        localPlayer.get()->UpdateState(localPlayer->getAnimstate(), data.viewangles);
        localPlayer.get()->UpdateClientSideAnimation();
        localPlayer.get()->ClientSideAnimation() = false;
        if ((data.sendPacket && (!AntiAim::lbyNextUpdatedPrevtick && !AntiAim::lbyNextUpdated)) || (data.sendPacket && (localPlayer->velocity().length2D() > 1.1f))) /*&& ( ^ !AntiAim::lbyNextUpdated))*/
        {
            backup_poses = localPlayer.get()->pose_parameters();
            backup_abs = localPlayer.get()->getAnimstate()->m_flGoalFeetYaw;
        }
    }
    localPlayer.get()->getAnimstate()->m_flFeetYawRate = 0.f;
    memory->setAbsAngle(localPlayer.get(), Vector{ 0,backup_abs,0 });
    localPlayer.get()->getAnimstate()->UnknownFraction = 0.f;
    std::memcpy(localPlayer.get()->animOverlays(), &networked_layers, sizeof(AnimationLayer) * localPlayer->getAnimationLayerCount());
    localPlayer.get()->pose_parameters() = backup_poses;
}


// Animfix

void Animations::players() noexcept
{

    if (!config->debug.Animfix)
        return;

    if (!localPlayer)
        return;

    for (int i = 1; i <= interfaces->engine->getMaxClients(); i++)
    {
        auto entity = interfaces->entityList->getEntity(i);
        if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive() || !entity->getAnimstate() || !entity->isOtherEnemy(localPlayer.get()))
            continue;

        if (!data.player[i].once)
        {
            data.player[i].poses = entity->pose_parameters();
            data.player[i].abs = entity->getAnimstate()->m_flGoalFeetYaw;
            data.player[i].simtime = 0;
            data.player[i].once = true;
        }

        constexpr auto timeToTicks = [](float time) {  return static_cast<int>(0.5f + time / memory->globalVars->intervalPerTick); };
        auto LPping = interfaces->engine->getNetworkChannel()->getLatency(0);

        if (data.player[i].networked_layers.empty())
            std::memcpy(&data.player[i].networked_layers, entity->animOverlays(), sizeof(AnimationLayer) * entity->getAnimationLayerCount());

        while (entity->getAnimstate()->m_iLastClientSideAnimationUpdateFramecount == memory->globalVars->framecount)
            entity->getAnimstate()->m_iLastClientSideAnimationUpdateFramecount -= 1;
        entity->InvalidateBoneCache();
        memory->setAbsOrigin(entity, entity->origin());
        *entity->getEffects() &= ~0x1000;

        if (config->debug.veloFix && (data.player[i].velocity.length2D() > 1.0f)) {
           // *entity->getAbsVelocity() = (data.player[i].velocity * ((timeToTicks(LPping) + data.player[i].averagefakelag)));
            *entity->getAbsVelocity() = entity->velocity();
        }
        else {
            *entity->getAbsVelocity() = entity->velocity();
        }




        
        std::memcpy(&data.player[i].networked_layers, entity->animOverlays(), sizeof(AnimationLayer) * entity->getAnimationLayerCount());
        entity->ClientSideAnimation() = true;
        /*
        if (entity->isOtherEnemy(localPlayer.get()) && config->debug.animstatedebug.resolver.enabled && data.player[i].chokedPackets >= 1 && localPlayer->isAlive())
            entity->getAnimstate()->m_flGoalFeetYaw = Resolver::CalculateFeet(entity);
        */
        entity->UpdateClientSideAnimation();
        entity->ClientSideAnimation() = false;

        //float backupSim = data.player[i].simtime;
        if (data.player[i].simtime != entity->simulationTime())
        {
            data.player[i].chokedPackets = static_cast<int>((entity->simulationTime() - data.player[i].simtime) / memory->globalVars->intervalPerTick) - 1;
            data.player[i].simtime = entity->simulationTime();
            data.player[i].poses = entity->pose_parameters();
            data.player[i].abs = entity->getAnimstate()->m_flGoalFeetYaw;
            /*If Update*/
            //Calculate average fake lag in the future
            //data.player[i].averagefakelag = data.player[i].chokedPackets; 
            //data.player[i].chokedPackets = 0;        
        }
        else {
            data.player[i].prevOrigin = entity->getAbsOrigin();
            data.player[i].velocity = entity->velocity();
        }


        entity->getAnimstate()->m_flFeetYawRate = 0.f;
        memory->setAbsAngle(entity, Vector{ 0,data.player[i].abs,0 });
        data.player[i].networked_layers[12].weight = std::numeric_limits<float>::epsilon();
        std::memcpy(entity->animOverlays(), &data.player[i].networked_layers, sizeof(AnimationLayer) * entity->getAnimationLayerCount());
        entity->pose_parameters() = data.player[i].poses;
        entity->InvalidateBoneCache();



        //if (config->debug.veloFix) {
        //    entity->setupBones(data.player[i].matrix, 256, 0x7FF00, data.player[i].simtime + (memory->globalVars->intervalPerTick * ((timeToTicks(LPping) + data.player[i].averagefakelag)))); // memory->globalVars->currenttime 
        //}
        //else {
        entity->setupBones(data.player[i].matrix, 256, 0x7FF00, memory->globalVars->currenttime);
        data.player[i].hasBackup = true;
        //}

        auto backup = data.lastest[i];
        auto boneCache = *(int**)(entity + 0x2910);
        auto countBones = *(int*)(entity + 0x291C);
        data.player[i].countBones = countBones;

        if (config->debug.veloFix && ((data.player[i].simtime != entity->simulationTime() || ((LPping * 1000) > 220)))) {


            if (data.player[i].averagefakelag < data.player[i].chokedPackets)
                data.player[i].averagefakelag = data.player[i].chokedPackets;

            auto LPping = interfaces->engine->getNetworkChannel()->getLatency(0);
            //LPping /= memory->globalVars->intervalPerTick;
            //auto timetoMove = (timeToTicks(((LPping/2) + (memory->globalVars->currenttime - data.player[i].simtime)) - .2f) * memory->globalVars->intervalPerTick);
            //entity->simulationTime() - data.player[i].simtime;

            auto timetoMove = timeToTicks(((LPping/2) + ((memory->globalVars->currenttime - .2f) - (data.player[i].simtime))) * memory->globalVars->intervalPerTick);
            if (timetoMove > 0.0f) {
                Vector newOrigin = data.player[i].prevOrigin + (data.player[i].velocity * timetoMove); // * (1 / 64)
                entity->setAbsOrigin(newOrigin);
            }
            else {
                auto timetoMove((LPping / 2) - .2f);
                if (timetoMove > 0.0f) {
                    Vector newOrigin = data.player[i].prevOrigin + (data.player[i].velocity * timetoMove); // * (1 / 64)
                    entity->setAbsOrigin(newOrigin);
                }
            }
            
        }

        backup.boneCache = boneCache;
        backup.countBones = countBones;
        backup.mins = entity->getCollideable()->obbMins();
        backup.max = entity->getCollideable()->obbMaxs();
        backup.origin = entity->getAbsOrigin();
    }
}


void Animations::setupMoveFix(Entity* entity) noexcept
{
    if (!config->debug.movefix)
        return; 

    if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive())
        return;

    if (entity->velocity().length2D() < 170.0f) {
        return;
    }

    auto backup = data.backup[entity->index()];
    auto boneCache = *(int**)(entity + 0x2910);
    auto countBones = *(int*)(entity + 0x291C);

    backup.boneCache = boneCache;
    backup.countBones = countBones;
    backup.mins = entity->getCollideable()->obbMins();
    backup.max = entity->getCollideable()->obbMaxs();
    backup.origin = entity->getAbsOrigin();
   

    Vector origin = entity->getAbsOrigin();

    entity->InvalidateBoneCache();
    memcpy(boneCache, backup.boneCache, sizeof(matrix3x4) * std::clamp(backup.countBones, 0, 256));


    if (entity->velocity().length2D() > 170.0f) {
            origin.x -= (entity->velocity().x * (3 / 64));
            origin.y -= (entity->velocity().y * (3 / 64));
            origin.z -= (entity->velocity().z * (3 / 64));
    }

    memory->setAbsOrigin(entity, origin);

}

void Animations::setup(Entity* entity, Backtrack::Record record) noexcept
{
    if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive())
        return;
    auto backup = data.backup[entity->index()];
    auto boneCache = *(int**)(entity + 0x2910);
    auto countBones = *(int*)(entity + 0x291C);

    backup.boneCache = boneCache;
    backup.countBones = countBones;
    backup.mins = entity->getCollideable()->obbMins();
    backup.max = entity->getCollideable()->obbMaxs();
    backup.origin = entity->getAbsOrigin();
    entity->InvalidateBoneCache();
    memcpy(boneCache, record.matrix, sizeof(matrix3x4) * std::clamp(countBones, 0, 256));

    entity->getCollideable()->obbMins() = record.mins;
    entity->getCollideable()->obbMaxs() = record.max;

    if (config->debug.movefix) {
        if (entity->velocity().length2D() > 170.0f) {
            record.origin.x -= (entity->velocity().x * (3 / 64));
            record.origin.y -= (entity->velocity().y * (3 / 64));
            record.origin.z -= (entity->velocity().z * (3 / 64));
        }
    }

    memory->setAbsOrigin(entity, record.origin);
    backup.hasBackup = true;
}

void Animations::finishSetup(Entity* entity) noexcept
{
    if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive())
        return;

    auto backup = data.lastest[entity->index()];
    if (data.backup[entity->index()].hasBackup)
    {
        auto boneCache = *(int**)(entity + 0x2910);
        entity->InvalidateBoneCache();
        memcpy(boneCache, backup.boneCache, sizeof(matrix3x4) * std::clamp(backup.countBones, 0, 256));

        entity->getCollideable()->obbMins() = backup.mins;
        entity->getCollideable()->obbMaxs() = backup.max;

        memory->setAbsOrigin(entity, backup.origin);
        data.backup[entity->index()].hasBackup = false;
    }
    else if (data.backupResolver[entity->index()].hasBackup)
    {
        auto boneCache = *(int**)(entity + 0x2910);
        entity->InvalidateBoneCache();
        memcpy(boneCache, backup.boneCache, sizeof(matrix3x4) * std::clamp(backup.countBones, 0, 256));

        entity->getCollideable()->obbMins() = backup.mins;
        entity->getCollideable()->obbMaxs() = backup.max;

        memory->setAbsOrigin(entity, backup.origin);
        data.backupResolver[entity->index()].hasBackup = false;
    }
}