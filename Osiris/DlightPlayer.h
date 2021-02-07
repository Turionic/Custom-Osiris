#pragma once

#include "IEffects.h"
#include "SDK/Entity.h"
#include "Memory.h"
#include "Interfaces.h"


namespace Player_Dlight {

    struct PDlight {
        bool hasBeenCreated = false;
        dlight_t* dlight = nullptr;
        dlight_t* elight = nullptr;
        dlight_t dlight_settings;
    };


    void SetUpPlayerLight(int i);
    void SetupLights();

    extern std::vector<PDlight> playerLights;

}