#include "IEffects.h"
#include "SDK/Entity.h"
#include "Memory.h"
#include "Interfaces.h"
#include "SDK/GlobalVars.h"
#include "DlightPlayer.h"

std::vector<Player_Dlight::PDlight> Player_Dlight::playerLights(65);

void Player_Dlight::SetUpPlayerLight(int i) {

    if (!config->visuals.dlight.enabled)
        return;

    Entity* entity = interfaces->entityList->getEntity(i);
    if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive()
        || !entity->isOtherEnemy(localPlayer.get()))
        return;

    dlight_t* dLight = interfaces->iveffects->CL_AllocDlight(entity->index());
    dlight_t* eLight = interfaces->iveffects->CL_AllocElight(entity->index());

    dLight->die = memory->globalVars->currenttime + 9999999.f;
    dLight->radius = dLight->color.exponent = config->visuals.dlightRadius;
    if (true) {
        dLight->color.r = (std::byte)(config->visuals.dlight.color[0] *255);
        dLight->color.g = (std::byte)(config->visuals.dlight.color[1] * 255);
        dLight->color.b = (std::byte)(config->visuals.dlight.color[2] * 255);
    }
    else {
        dLight->color.r = (std::byte)2;
        dLight->color.g = (std::byte)48;
        dLight->color.b = (std::byte)22;
    }
    dLight->color.exponent = config->visuals.dlightExponent;
    dLight->flags = 0;
    dLight->key = entity->index();
    dLight->decay = 20.0f;
    dLight->m_Direction = entity->getBonePosition(0);
    dLight->origin = entity->getBonePosition(0);
    *eLight = *dLight;
    playerLights.at(entity->index()).hasBeenCreated = true;
    playerLights.at(entity->index()).dlight = dLight;
    playerLights.at(entity->index()).elight = eLight;
    playerLights.at(entity->index()).dlight_settings = *dLight;
}


void Player_Dlight::SetupLights() {
    for (int i = 0; i <= interfaces->engine->getMaxClients(); i++) {
        Entity* entity = interfaces->entityList->getEntity(i);
        if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive()
            || !entity->isOtherEnemy(localPlayer.get())) {
            if ((i > 0) && (i < 65)) {
                if (playerLights.at(i).hasBeenCreated) {
                    playerLights.at(entity->index()).dlight->die = memory->globalVars->currenttime;
                    playerLights.at(entity->index()).elight->die = memory->globalVars->currenttime;
                }
                playerLights.at(i).hasBeenCreated = false;
            }
            continue;
        }

        if (!playerLights.at(entity->index()).hasBeenCreated){
            SetUpPlayerLight(entity->index());
        }
        else {
            playerLights.at(entity->index()).dlight_settings.origin = entity->getBonePosition(0);
            playerLights.at(entity->index()).dlight_settings.die = memory->globalVars->currenttime + 9999999.f;
            playerLights.at(entity->index()).dlight_settings.m_Direction = entity->getBonePosition(0);
            playerLights.at(entity->index()).dlight_settings.color.r = (std::byte)(config->visuals.dlight.color[0] * 255);
            playerLights.at(entity->index()).dlight_settings.color.g = (std::byte)(config->visuals.dlight.color[1] * 255);
            playerLights.at(entity->index()).dlight_settings.color.b = (std::byte)(config->visuals.dlight.color[2] * 255);
            playerLights.at(entity->index()).dlight_settings.radius = config->visuals.dlightRadius;
            playerLights.at(entity->index()).dlight_settings.color.exponent = config->visuals.dlightExponent;
            *(playerLights.at(entity->index()).dlight) = playerLights.at(entity->index()).dlight_settings;
            *(playerLights.at(entity->index()).elight) = playerLights.at(entity->index()).dlight_settings;

        }


    }
}
