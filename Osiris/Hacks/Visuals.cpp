#include "../fnv.h"
#include "Visuals.h"

#include "../SDK/ConVar.h"
#include "../SDK/Entity.h"
#include "../SDK/FrameStage.h"
#include "../SDK/GameEvent.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/Input.h"
#include "../SDK/Material.h"
#include "../SDK/MaterialSystem.h"
#include "../SDK/NetworkStringTable.h"
#include "../SDK/RenderContext.h"
#include "../SDK/Surface.h"
#include "../SDK/ModelInfo.h"
#include "../SDK/Beams.h"


#include <array>

void Visuals::playerModel(FrameStage stage) noexcept
{
    if (stage != FrameStage::RENDER_START && stage != FrameStage::RENDER_END)
        return;

    static int originalIdx = 0;

    if (!localPlayer) {
        originalIdx = 0;
        return;
    }

    constexpr auto getModel = [](int team) constexpr noexcept -> const char* {
        constexpr std::array models{
        "models/player/custom_player/legacy/ctm_fbi_variantb.mdl",
        "models/player/custom_player/legacy/ctm_fbi_variantf.mdl",
        "models/player/custom_player/legacy/ctm_fbi_variantg.mdl",
        "models/player/custom_player/legacy/ctm_fbi_varianth.mdl",
        "models/player/custom_player/legacy/ctm_sas_variantf.mdl",
        "models/player/custom_player/legacy/ctm_st6_variante.mdl",
        "models/player/custom_player/legacy/ctm_st6_variantg.mdl",
        "models/player/custom_player/legacy/ctm_st6_varianti.mdl",
        "models/player/custom_player/legacy/ctm_st6_variantk.mdl",
        "models/player/custom_player/legacy/ctm_st6_variantm.mdl",
        "models/player/custom_player/legacy/tm_balkan_variantf.mdl",
        "models/player/custom_player/legacy/tm_balkan_variantg.mdl",
        "models/player/custom_player/legacy/tm_balkan_varianth.mdl",
        "models/player/custom_player/legacy/tm_balkan_varianti.mdl",
        "models/player/custom_player/legacy/tm_balkan_variantj.mdl",
        "models/player/custom_player/legacy/tm_leet_variantf.mdl",
        "models/player/custom_player/legacy/tm_leet_variantg.mdl",
        "models/player/custom_player/legacy/tm_leet_varianth.mdl",
        "models/player/custom_player/legacy/tm_leet_varianti.mdl",
        "models/player/custom_player/legacy/tm_phoenix_variantf.mdl",
        "models/player/custom_player/legacy/tm_phoenix_variantg.mdl",
        "models/player/custom_player/legacy/tm_phoenix_varianth.mdl",
        
        "models/player/custom_player/legacy/tm_pirate.mdl",
        "models/player/custom_player/legacy/tm_pirate_varianta.mdl",
        "models/player/custom_player/legacy/tm_pirate_variantb.mdl",
        "models/player/custom_player/legacy/tm_pirate_variantc.mdl",
        "models/player/custom_player/legacy/tm_pirate_variantd.mdl",
        "models/player/custom_player/legacy/tm_anarchist.mdl",
        "models/player/custom_player/legacy/tm_anarchist_varianta.mdl",
        "models/player/custom_player/legacy/tm_anarchist_variantb.mdl",
        "models/player/custom_player/legacy/tm_anarchist_variantc.mdl",
        "models/player/custom_player/legacy/tm_anarchist_variantd.mdl",
        "models/player/custom_player/legacy/tm_balkan_varianta.mdl",
        "models/player/custom_player/legacy/tm_balkan_variantb.mdl",
        "models/player/custom_player/legacy/tm_balkan_variantc.mdl",
        "models/player/custom_player/legacy/tm_balkan_variantd.mdl",
        "models/player/custom_player/legacy/tm_balkan_variante.mdl",
        "models/player/custom_player/legacy/tm_jumpsuit_varianta.mdl",
        "models/player/custom_player/legacy/tm_jumpsuit_variantb.mdl",
        "models/player/custom_player/legacy/tm_jumpsuit_variantc.mdl"
        };

        switch (team) {
        case 2: return static_cast<std::size_t>(config->visuals.playerModelT - 1) < models.size() ? models[config->visuals.playerModelT - 1] : nullptr;
        case 3: return static_cast<std::size_t>(config->visuals.playerModelCT - 1) < models.size() ? models[config->visuals.playerModelCT - 1] : nullptr;
        default: return nullptr;
        }
    };

    if (const auto model = getModel(localPlayer->team())) {
        if (stage == FrameStage::RENDER_START) {
            originalIdx = localPlayer->modelIndex();
            if (const auto modelprecache = interfaces->networkStringTableContainer->findTable("modelprecache")) {
                modelprecache->addString(false, model);
                const auto viewmodelArmConfig = memory->getPlayerViewmodelArmConfigForPlayerModel(model);
                modelprecache->addString(false, viewmodelArmConfig[2]);
                modelprecache->addString(false, viewmodelArmConfig[3]);
            }
        }

        const auto idx = stage == FrameStage::RENDER_END && originalIdx ? originalIdx : interfaces->modelInfo->getModelIndex(model);

        localPlayer->setModelIndex(idx);

        if (const auto ragdoll = interfaces->entityList->getEntityFromHandle(localPlayer->ragdoll()))
            ragdoll->setModelIndex(idx);
    }
}

#include <sstream>
#include <codecvt>
#include <locale>

#include "Chams.h"
#include "../SDK/Material.h"
#include "../SDK/MaterialSystem.h"
#include "../SDK/StudioRender.h"
#include "../SDK/KeyValues.h"
void Visuals::colorWorld() noexcept
{

    if (!config->visuals.world.enabled && !config->visuals.sky.enabled && !config->visuals.props.enabled && !config->visuals.smokecolor.enabled && !config->visuals.smoketrans
        && !config->visuals.PrecacheWorld.enabled && !config->visuals.PrecacheProps.enabled && !config->visuals.NonCached.enabled && !config->visuals.NonCachedWorld.enabled && !config->visuals.all.enabled) {
        return;
    }

    if (!localPlayer || localPlayer->isDormant() || !localPlayer->isAlive())
        return;

    /*
    Material* normal = interfaces->materialSystem->createMaterial("normal", KeyValues::fromString("VertexLitGeneric", nullptr));
    Material* flat = interfaces->materialSystem->createMaterial("flat", KeyValues::fromString("UnlitGeneric", nullptr));
    Material* chrome = interfaces->materialSystem->createMaterial("chrome", KeyValues::fromString("VertexLitGeneric", "$envmap env_cubemap"));
    Material* glow = interfaces->materialSystem->createMaterial("glow", KeyValues::fromString("VertexLitGeneric", "$additive 1 $envmap models/effects/cube_white $envmapfresnel 1 $alpha .8"));
    Material* pearlescent = interfaces->materialSystem->createMaterial("pearlescent", KeyValues::fromString("VertexLitGeneric", "$ambientonly 1 $phong 1 $pearlescent 3 $basemapalphaphongmask 1"));
    Material* metallic = interfaces->materialSystem->createMaterial("metallic", KeyValues::fromString("VertexLitGeneric", "$basetexture white $ignorez 0 $envmap env_cubemap $normalmapalphaenvmapmask 1 $envmapcontrast 1 $nofog 1 $model 1 $nocull 0 $selfillum 1 $halfambert 1 $znearer 0 $flat 1"));
    */ // TODO: Implement Chams for world

    if (!interfaces->engine->isInGame())
        return;

    for (short h = interfaces->materialSystem->firstMaterial(); h != interfaces->materialSystem->invalidMaterial(); h = interfaces->materialSystem->nextMaterial(h)) {
        const auto mat = interfaces->materialSystem->getMaterial(h);

        if (!mat)
            continue;


        const std::string_view textureGroup = mat->getTextureGroupName();



        if ((config->visuals.PrecacheWorld.enabled && (textureGroup.starts_with("World") && mat->isPrecached()))) {
            if (config->visuals.PrecacheWorld.rainbow)
                mat->colorModulate(rainbowColor(config->visuals.PrecacheWorld.rainbowSpeed));
            else
                mat->colorModulate(config->visuals.PrecacheWorld.color);
        } else if ((config->visuals.NonCachedWorld.enabled && (textureGroup.starts_with("World") && !mat->isPrecached()))) {
            if (config->visuals.NonCachedWorld.rainbow)
                mat->colorModulate(rainbowColor(config->visuals.NonCachedWorld.rainbowSpeed));
            else
                mat->colorModulate(config->visuals.NonCachedWorld.color);
        }
        else if (config->visuals.world.enabled && (textureGroup.starts_with("World") && (!textureGroup.starts_with("StaticProp")))) {

            //char* groupname = (char*)mat->getTextureGroupName();

            //std::string cmd{ "say " + (std::string)groupname };
            //interfaces->engine->clientCmdUnrestricted(cmd.c_str());

            if (config->visuals.world.rainbow) {
                /*
                auto mat = interfaces->materialSystem->createMaterial("c4_gift", KeyValues::fromString("VertexLitGeneric", "$basetexture models/destruction_tanker/blackcable $normalmapalphaenvmapmask 1"));
                if (!mat)
                    continue;
                interfaces->studioRender->forcedMaterialOverride(mat);
                */
                mat->colorModulate(rainbowColor(config->visuals.world.rainbowSpeed));

            }
            else
                mat->colorModulate(config->visuals.world.color);
        }
        else if (config->visuals.sky.enabled && textureGroup.starts_with("SkyBox")) {
            if (config->visuals.sky.rainbow)
                mat->colorModulate(rainbowColor(config->visuals.sky.rainbowSpeed));
            else
                mat->colorModulate(config->visuals.sky.color);
        }
        else if (config->visuals.PrecacheProps.enabled && textureGroup.starts_with("StaticProp") && mat->isPrecached()) {
            ConVar* static_Prop = interfaces->cvar->findVar("r_DrawSpecificStaticProp");
            static_Prop->setValue(0);

            if (config->visuals.PrecacheProps.rainbow)
                mat->colorModulate(rainbowColor(config->visuals.PrecacheProps.rainbowSpeed));
            else
                mat->colorModulate(config->visuals.PrecacheProps.color);
        }
        else if (config->visuals.NonCached.enabled && textureGroup.starts_with("StaticProp") && !mat->isPrecached()) {
            ConVar* static_Prop = interfaces->cvar->findVar("r_DrawSpecificStaticProp");
            static_Prop->setValue(0);

            if (config->visuals.NonCached.rainbow)
                mat->colorModulate(rainbowColor(config->visuals.NonCached.rainbowSpeed));
            else
                mat->colorModulate(config->visuals.NonCached.color);
        }
        else if (config->visuals.props.enabled && textureGroup.starts_with("StaticProp")) {
            ConVar* static_Prop = interfaces->cvar->findVar("r_DrawSpecificStaticProp");
            static_Prop->setValue(0);

            if (config->visuals.NonCached.rainbow)
                mat->colorModulate(rainbowColor(config->visuals.NonCached.rainbowSpeed));
            else
                mat->colorModulate(config->visuals.NonCached.color);
        }
        else if (config->visuals.all.enabled) {
            mat->colorModulate(config->visuals.all.color);
        }

    }
    if (config->visuals.smokecolor.enabled || config->visuals.smoketrans) {
        constexpr std::array smokeMaterials{
            "particle/vistasmokev1/vistasmokev1_emods",
            "particle/vistasmokev1/vistasmokev1_emods_impactdust",
            "particle/vistasmokev1/vistasmokev1_fire",
            "particle/vistasmokev1/vistasmokev1_smokegrenade",
            "particle/vistasmokev1/vistasmokev1",
            "particle/smokegrenade",
            "particle/smokegrenade1",
            "particle/smokegrenade2",
            "particle/smokegrenade3",
            "particle/smokegrenade_2",
            "particle/smokegrenade_sc",
            "particle/smokegrenade1",
            "particle/vistasmokev1_add_nearcull",
            "particle/smokesprites001",
            "effects/smoke/smoke_cloud",
            "effects/smoke/smoke_cloud_detail"
        };

        for (const auto mat : smokeMaterials) {
            const auto material = interfaces->materialSystem->findMaterial(mat);
            if (config->visuals.smokecolor.enabled) {
                material->colorModulate(config->visuals.smokecolor.color);
            }
            if (config->visuals.smoketrans) {
                material->alphaModulate((config->visuals.smokealpha / 1000.0f));
            }
        }
    }

}

void Visuals::transparentWorld() noexcept
{
    if (!config->visuals.worldtrans && !config->visuals.proptrans)
       return;

    if (!interfaces->engine->isInGame())
        return;

    if (!localPlayer || localPlayer->isDormant() || !localPlayer->isAlive())
        return;

    for (short h = interfaces->materialSystem->firstMaterial(); h != interfaces->materialSystem->invalidMaterial(); h = interfaces->materialSystem->nextMaterial(h)) {
        const auto mat = interfaces->materialSystem->getMaterial(h);

        if (!mat || ((!config->visuals.Ignore_precache && !mat->isPrecached()) ^ (config->visuals.Precache_Only && mat->isPrecached())))
           continue;


        const std::string_view textureGroup = mat->getTextureGroupName();

        if ((textureGroup.starts_with("World") && config->visuals.worldtrans)) {
                mat->alphaModulate((config->visuals.alphaval / 1000.0f));
        }
        if ((textureGroup.starts_with("StaticProp") && config->visuals.proptrans)) {
            
            ConVar* static_Prop = interfaces->cvar->findVar("r_DrawSpecificStaticProp");
            static_Prop->setValue(0);    

            mat->alphaModulate((config->visuals.propalphaval / 1000.0f));
        }

    }
}

void Visuals::NightModeExtended() noexcept {
    if (!config->visuals.NightMode.enabled)
        return;
    if (!interfaces->engine->isInGame())
        return;

    if (config->visuals.matgrey) {
        ConVar* mat_grey = interfaces->cvar->findVar("mat_drawgray");
        mat_grey->setValue(1);
    }
    if (!localPlayer || localPlayer->isDormant() || !localPlayer->isAlive())
        return;
    ConVar* ambilight_r = interfaces->cvar->findVar("mat_ambient_light_r");
    ConVar* ambilight_g = interfaces->cvar->findVar("mat_ambient_light_g");
    ConVar* ambilight_b = interfaces->cvar->findVar("mat_ambient_light_b");

    if (config->visuals.NightMode.rainbow) {
        const auto [r, g, b] = rainbowColor(config->visuals.NightMode.rainbowSpeed);
        ambilight_r->setValue(r);
        ambilight_g->setValue(g);
        ambilight_b->setValue(b);
    }
    else {
        ambilight_r->setValue(config->visuals.NightMode.color[0]);
        ambilight_g->setValue(config->visuals.NightMode.color[1]);
        ambilight_b->setValue(config->visuals.NightMode.color[2]);
    }
}

void Visuals::modifySmoke(FrameStage stage) noexcept
{
    if ((stage != FrameStage::RENDER_START) && (stage != FrameStage::RENDER_END))
        return;

    if (!interfaces->engine->isInGame())
        return;

    if (!localPlayer || localPlayer->isDormant() || !localPlayer->isAlive())
        return;


    //if(!memory->globalVars-)

    constexpr std::array smokeMaterials{
            "particle/vistasmokev1/vistasmokev1_emods",
            "particle/vistasmokev1/vistasmokev1_emods_impactdust",
            "particle/vistasmokev1/vistasmokev1_fire",
            "particle/vistasmokev1/vistasmokev1_smokegrenade",
            "particle/vistasmokev1/vistasmokev1",
            "particle/particle_smokegrenade",
            "particle/particle_smokegrenade1",
            "particle/particle_smokegrenade2",
            "particle/particle_smokegrenade3",
            "particle/particle_smokegrenade_2",
            "particle/particle_smokegrenade_sc",
            "particle/particle_smokegrenade1",
            "particle/vistasmokev1_add_nearcull",
            "particle/smokesprites001",
            "effects/smoke/smoke_cloud",
            "effects/smoke/smoke_cloud_detail"
        };
    

    //constexpr std::array smokeMaterials{
    //    "particle/vistasmokev1/vistasmokev1_emods",
    //    "particle/vistasmokev1/vistasmokev1_emods_impactdust",
    //    "particle/vistasmokev1/vistasmokev1_fire",
    //    "particle/vistasmokev1/vistasmokev1_smokegrenade"
    //};

    for (const auto mat : smokeMaterials) {

        ConVar* static_Prop = interfaces->cvar->findVar("r_DrawSpecificStaticProp");
        static_Prop->setValue(0);

        const auto material = interfaces->materialSystem->findMaterial(mat);
        if (config->visuals.smokecolor.enabled) {
            material->setMaterialVarFlag(MaterialVarFlag::MATERIAL_VAR_ADDITIVE, true);
            material->colorModulate(config->visuals.smokecolor.color);
        }
        if (config->visuals.smoketrans) {
            material->setMaterialVarFlag(MaterialVarFlag::MATERIAL_VAR_ADDITIVE, true);
            material->alphaModulate((config->visuals.smokealpha / 1000.0f));
        }
        material->setMaterialVarFlag(MaterialVarFlag::NO_DRAW, stage == FrameStage::RENDER_START && config->visuals.noSmoke);
        material->setMaterialVarFlag(MaterialVarFlag::WIREFRAME, stage == FrameStage::RENDER_START && config->visuals.wireframeSmoke);
        char flag  = 1 << config->visuals.customflag;
        material->setMaterialVarFlag((MaterialVarFlag)(flag), stage == FrameStage::RENDER_START && config->visuals.smokeflagmanual);
    }
}

void Visuals::thirdperson() noexcept
{
    static bool isInThirdperson{ true };
    static float lastTime{ 0.0f };

    if (GetAsyncKeyState(config->visuals.thirdpersonKey) && memory->globalVars->realtime - lastTime > 0.5f) {
        isInThirdperson = !isInThirdperson;
        lastTime = memory->globalVars->realtime;
    }

    if (config->visuals.thirdperson)
        if (memory->input->isCameraInThirdPerson = (!config->visuals.thirdpersonKey || isInThirdperson)
            && localPlayer && localPlayer->isAlive())
            memory->input->cameraOffset.z = static_cast<float>(config->visuals.thirdpersonDistance);
}

void Visuals::removeVisualRecoil(FrameStage stage) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    static Vector aimPunch;
    static Vector viewPunch;

    if (stage == FrameStage::RENDER_START) {
        aimPunch = localPlayer->aimPunchAngle();
        viewPunch = localPlayer->viewPunchAngle();

        if (config->visuals.noAimPunch && !config->misc.recoilCrosshair)
            localPlayer->aimPunchAngle() = Vector{ };

        if (config->visuals.noViewPunch)
            localPlayer->viewPunchAngle() = Vector{ };

    } else if (stage == FrameStage::RENDER_END) {
        localPlayer->aimPunchAngle() = aimPunch;
        localPlayer->viewPunchAngle() = viewPunch;
    }
}

void Visuals::removeBlur(FrameStage stage) noexcept
{
    if (stage != FrameStage::RENDER_START && stage != FrameStage::RENDER_END)
        return;

    static auto blur = interfaces->materialSystem->findMaterial("dev/scope_bluroverlay");
    blur->setMaterialVarFlag(MaterialVarFlag::NO_DRAW, stage == FrameStage::RENDER_START && config->visuals.noBlur);
}

void Visuals::updateBrightness() noexcept
{
    static auto brightness = interfaces->cvar->findVar("mat_force_tonemap_scale");
    brightness->setValue(config->visuals.brightness);
}

void Visuals::removeGrass(FrameStage stage) noexcept
{
    if (stage != FrameStage::RENDER_START && stage != FrameStage::RENDER_END)
        return;

    constexpr auto getGrassMaterialName = []() noexcept -> const char* {
        switch (fnv::hashRuntime(interfaces->engine->getLevelName())) {
        case fnv::hash("dz_blacksite"): return "detail/detailsprites_survival";
        case fnv::hash("dz_sirocco"): return "detail/dust_massive_detail_sprites";
        case fnv::hash("dz_junglety"): return "detail/tropical_grass";
        default: return nullptr;
        }
    };

    if (const auto grassMaterialName = getGrassMaterialName())
        interfaces->materialSystem->findMaterial(grassMaterialName)->setMaterialVarFlag(MaterialVarFlag::NO_DRAW, stage == FrameStage::RENDER_START && config->visuals.noGrass);
}

void Visuals::remove3dSky() noexcept
{
    static auto sky = interfaces->cvar->findVar("r_3dsky");
    sky->setValue(!config->visuals.no3dSky);
}

void Visuals::removeShadows() noexcept
{
    static auto shadows = interfaces->cvar->findVar("cl_csm_enabled");
    shadows->setValue(!config->visuals.noShadows);
}


void Visuals::NoScopeInZoom(FrameStage stage) noexcept {
    //config->visuals.thirdpersonDistance;
    //config->visuals.fov
    /*
    if (stage == FrameStage::RENDER_START) {
        if (localPlayer->isScoped()) {
            localPlayer->fov() = config->visuals.fov;
        }
    }
    */

}

void Visuals::applyZoom(FrameStage stage) noexcept
{
    if (config->visuals.zoom && localPlayer) {
        if (stage == FrameStage::RENDER_START && (localPlayer->fov() == 90 || localPlayer->fovStart() == 90)) {
            static bool scoped{ false };

            if (GetAsyncKeyState(config->visuals.zoomKey) & 1)
                scoped = !scoped;

            if (scoped) {
                localPlayer->fov() = 40;
                localPlayer->fovStart() = 40;
            }
        }
    }
}

#define DRAW_SCREEN_EFFECT(material) \
{ \
    const auto drawFunction = memory->drawScreenEffectMaterial; \
    int w, h; \
    interfaces->surface->getScreenSize(w, h); \
    __asm { \
        __asm push h \
        __asm push w \
        __asm push 0 \
        __asm xor edx, edx \
        __asm mov ecx, material \
        __asm call drawFunction \
        __asm add esp, 12 \
    } \
}

void Visuals::applyScreenEffects() noexcept
{
    if (!config->visuals.screenEffect)
        return;

    const auto material = interfaces->materialSystem->findMaterial([] {
        constexpr std::array effects{
            "effects/dronecam",
            "effects/underwater_overlay",
            "effects/healthboost",
            "effects/dangerzone_screen"
        };

        if (config->visuals.screenEffect <= 2 || static_cast<std::size_t>(config->visuals.screenEffect - 2) >= effects.size())
            return effects[0];
        return effects[config->visuals.screenEffect - 2];
    }());

    if (config->visuals.screenEffect == 1)
        material->findVar("$c0_x")->setValue(0.0f);
    else if (config->visuals.screenEffect == 2)
        material->findVar("$c0_x")->setValue(0.1f);
    else if (config->visuals.screenEffect >= 4)
        material->findVar("$c0_x")->setValue(1.0f);

    DRAW_SCREEN_EFFECT(material)
}

void Visuals::hitEffect(GameEvent* event) noexcept
{


    if (config->visuals.hitEffect && localPlayer) {
        static float lastHitTime = 0.0f;

        if (event && interfaces->engine->getPlayerForUserID(event->getInt("attacker")) == localPlayer->index()) {
            lastHitTime = memory->globalVars->realtime;
            return;
        }

        if (lastHitTime + config->visuals.hitEffectTime >= memory->globalVars->realtime) {
            constexpr auto getEffectMaterial = [] {
                static constexpr const char* effects[]{
                "effects/dronecam",
                "effects/underwater_overlay",
                "effects/healthboost",
                "effects/dangerzone_screen"
                };

                if (config->visuals.hitEffect <= 2)
                    return effects[0];
                return effects[config->visuals.hitEffect - 2];
            };

           
            auto material = interfaces->materialSystem->findMaterial(getEffectMaterial());
            if (config->visuals.hitEffect == 1)
                material->findVar("$c0_x")->setValue(0.0f);
            else if (config->visuals.hitEffect == 2)
                material->findVar("$c0_x")->setValue(0.1f);
            else if (config->visuals.hitEffect >= 4)
                material->findVar("$c0_x")->setValue(1.0f);

            DRAW_SCREEN_EFFECT(material)
        }
    }
}

void Visuals::hitMarker(GameEvent* event) noexcept
{
    if (config->visuals.hitMarker == 0 || !localPlayer)
        return;

    static float lastHitTime = 0.0f;

    if (event && interfaces->engine->getPlayerForUserID(event->getInt("attacker")) == localPlayer->index()) {
        lastHitTime = memory->globalVars->realtime;
        return;
    }

    if (lastHitTime + config->visuals.hitMarkerTime < memory->globalVars->realtime)
        return;

    switch (config->visuals.hitMarker) {
    case 1:
        const auto [width, height] = interfaces->surface->getScreenSize();

        const auto width_mid = width / 2;
        const auto height_mid = height / 2;

        interfaces->surface->setDrawColor(255, 255, 255, 255);
        interfaces->surface->drawLine(width_mid + 10, height_mid + 10, width_mid + 4, height_mid + 4);
        interfaces->surface->drawLine(width_mid - 10, height_mid + 10, width_mid - 4, height_mid + 4);
        interfaces->surface->drawLine(width_mid + 10, height_mid - 10, width_mid + 4, height_mid - 4);
        interfaces->surface->drawLine(width_mid - 10, height_mid - 10, width_mid - 4, height_mid - 4);
        break;
    }
}

void Visuals::disablePostProcessing(FrameStage stage) noexcept
{
    if (stage != FrameStage::RENDER_START && stage != FrameStage::RENDER_END)
        return;

    *memory->disablePostProcessing = stage == FrameStage::RENDER_START && config->visuals.disablePostProcessing;
}

void Visuals::reduceFlashEffect() noexcept
{
    if (localPlayer)
        localPlayer->flashMaxAlpha() = 255.0f - config->visuals.flashReduction * 2.55f;
}

bool Visuals::removeHands(const char* modelName) noexcept
{
    return config->visuals.noHands && std::strstr(modelName, "arms") && !std::strstr(modelName, "sleeve");
}

bool Visuals::removeSleeves(const char* modelName) noexcept
{
    return config->visuals.noSleeves && std::strstr(modelName, "sleeve");
}


bool Visuals::removeWeapons(const char* modelName) noexcept
{

    return config->visuals.noWeapons && std::strstr(modelName, "models/weapons/v_")
        && !std::strstr(modelName, "arms") && !std::strstr(modelName, "tablet")
        && !std::strstr(modelName, "parachute") && !std::strstr(modelName, "fists");
}

void Visuals::skybox(FrameStage stage) noexcept
{
    if (stage != FrameStage::RENDER_START && stage != FrameStage::RENDER_END)
        return;

    constexpr std::array skyboxes{ "cs_baggage_skybox_", "cs_tibet", "embassy", "italy", "jungle", "nukeblank", "office", "sky_cs15_daylight01_hdr", "sky_cs15_daylight02_hdr", "sky_cs15_daylight03_hdr", "sky_cs15_daylight04_hdr", "sky_csgo_cloudy01", "sky_csgo_night_flat", "sky_csgo_night02", "sky_day02_05_hdr", "sky_day02_05", "sky_dust", "sky_l4d_rural02_ldr", "sky_venice", "vertigo_hdr", "vertigo", "vertigoblue_hdr", "vietnam", "sky_lunacy", "sky_borealis01", "cliff", "blue", "city1", "neb1","xen8", "sky2mh_"};

    if (stage == FrameStage::RENDER_START && static_cast<std::size_t>(config->visuals.skybox - 1) < skyboxes.size()) {
        memory->loadSky(skyboxes[config->visuals.skybox - 1]);
    } else {
        static const auto sv_skyname = interfaces->cvar->findVar("sv_skyname");
        memory->loadSky(sv_skyname->string);
    }
}

void Visuals::bulletBeams(GameEvent* event) noexcept
{
    if (!config->visuals.bulletTracers.enabled || !interfaces->engine->isInGame() || !interfaces->engine->isConnected() || memory->renderBeams == nullptr)
        return;

    const auto player = interfaces->entityList->getEntity(interfaces->engine->getPlayerForUserID(event->getInt("userid")));

    if (!player || !localPlayer)
        return;

    if(((player == localPlayer.get()) && !config->visuals.bulletTracers.enabled))
        return;

    if (localPlayer->isOtherEnemy(player) && !config->visuals.bulletTracersEnemy.enabled)
        return;



    Vector position;
    position.x = event->getFloat("x");
    position.y = event->getFloat("y");
    position.z = event->getFloat("z");

    BeamInfo_t beam_info;
    beam_info.m_nType = TE_BEAMPOINTS;
    //beam_info.m_pszModelName = "sprites/physbeam.vmt";
    //beam_info.m_nModelIndex = -1;
    beam_info.m_pszModelName = "sprites/purplelaser1.vmt";
    beam_info.m_nModelIndex = interfaces->modelInfo->getModelIndex("sprites/purplelaser1.vmt");
    beam_info.m_flHaloScale = 0.f;
    beam_info.m_flLife = player->isOtherEnemy(localPlayer.get()) ? .8f : 1.5f;
    beam_info.m_flWidth = 1.f;
    beam_info.m_flEndWidth = 1.f;
    beam_info.m_flFadeLength = 0.1f;
    beam_info.m_flAmplitude = 2.f;
    beam_info.m_flBrightness = 255.f;
    beam_info.m_flSpeed = 0.2f;
    beam_info.m_nStartFrame = 0;
    beam_info.m_flFrameRate = 0.f;
    beam_info.m_flRed = player->isOtherEnemy(localPlayer.get()) ? config->visuals.bulletTracersEnemy.color[0] * 255 : config->visuals.bulletTracers.color[0] * 255;
    beam_info.m_flGreen = player->isOtherEnemy(localPlayer.get()) ? config->visuals.bulletTracersEnemy.color[1] * 255 : config->visuals.bulletTracers.color[1] * 255;
    beam_info.m_flBlue = player->isOtherEnemy(localPlayer.get()) ? config->visuals.bulletTracersEnemy.color[2] * 255 : config->visuals.bulletTracers.color[2] * 255;
    beam_info.m_nSegments = config->visuals.beamseg;


    beam_info.m_bRenderable = true;
    beam_info.m_nFlags = FBEAM_ONLYNOISEONCE | FBEAM_NOTILE | FBEAM_HALOBEAM;

    // create beam backwards because it looks nicer.
    beam_info.m_vecStart = position;
    beam_info.m_vecEnd = player->getEyePosition();

    auto beam = memory->renderBeams->CreateBeamPoints(beam_info);
    if (beam)
        memory->renderBeams->DrawBeam(beam);


    if (player->isOtherEnemy(localPlayer.get())) {
        beam_info.m_pszModelName = "sprites/physbeam.vmt";
        beam_info.m_nModelIndex = -1;
        beam_info.m_flLife = .7f;
        auto beam = memory->renderBeams->CreateBeamPoints(beam_info);
        if (beam)
            memory->renderBeams->DrawBeam(beam);
    }

}

#include "StreamProofESP.h"
#include "../ConfigStructs.h"

void Visuals::grenadeBeams(GameEvent* event) noexcept
{
    // !config->visuals.grenadeBeams.enabled ||
    if (!interfaces->engine->isInGame() || !interfaces->engine->isConnected() || memory->renderBeams == nullptr)
        return;

    const auto player = interfaces->entityList->getEntity(interfaces->engine->getPlayerForUserID(event->getInt("userid")));
    
    
    //std::vector<Vector> player_points;

    if (!player || !localPlayer || (player != localPlayer.get() && !player->isOtherEnemy(localPlayer.get())))
        return;

    Vector position;
    position.x = event->getFloat("x");
    position.y = event->getFloat("y");
    position.z = event->getFloat("z");


    

   

    for (int i = 1; i <= interfaces->engine->getMaxClients(); i++) {
        auto entity = interfaces->entityList->getEntity(i);
        if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive() || !entity->isOtherEnemy(localPlayer.get())) {
            continue;
        }
        StreamProofESP::DrawItem Item;

        Item.type = StreamProofESP::LINE;
        Item.Fade = true;
        Item.StartPos = position;
        Item.BoundType = 1;
        Item.boundstartEntity = entity;
        Item.exist_time = 10.0f;
        Item.thickness = 2.0f;
        ColorA color;
        color.color[0] = config->visuals.grenadeBeams.color[0];
        color.color[1] = config->visuals.grenadeBeams.color[1];
        color.color[2] = config->visuals.grenadeBeams.color[2];
        color.color[3] = 1.0f;


        if (entity->isFlashed()) {
            color.color[0] = 1.0f;
            color.color[1] = 1.0f;
            color.color[2] = 1.0f;
        }
        else if (entity->health() < 20) {
            color.color[0] = 1.0f;
            color.color[1] = 0;
            color.color[2] = 0;
        }

        Item.Color = color;



        StreamProofESP::ESPItemList.push_back(Item);

    }
    /*
    BeamInfo_t beam_info;
    beam_info.m_nType = TE_BEAMPOINTS;
    beam_info.m_pszModelName = "sprites/glow01.vmt";
    beam_info.m_nModelIndex = -1;
    beam_info.m_flHaloScale = 1.f;
    beam_info.m_flLife = 10.f;
    beam_info.m_flWidth = 1.f;
    beam_info.m_flEndWidth = 1.f;
    beam_info.m_flFadeLength = 0.1f;
    beam_info.m_flAmplitude = 2.f;
    beam_info.m_flBrightness = 255.f;
    beam_info.m_flSpeed = 0.1f;
    beam_info.m_nStartFrame = 0;
    beam_info.m_flFrameRate = 0.f;
    beam_info.m_flRed = config->visuals.grenadeBeams.color[0] * 255;
    beam_info.m_flGreen = config->visuals.grenadeBeams.color[1] * 255;
    beam_info.m_flBlue = config->visuals.grenadeBeams.color[2] * 255;
    beam_info.m_nSegments = 35;
    beam_info.m_bRenderable = true;
    beam_info.m_nFlags = FBEAM_ONLYNOISEONCE | FBEAM_NOTILE | FBEAM_HALOBEAM;
    beam_info.m_nType = TE_BEAMPOINTS;
    // create beam backwards because it looks nicer.

    for (int i = 1; i <= interfaces->engine->getMaxClients(); i++) {
        auto entity = interfaces->entityList->getEntity(i);
        if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive() || !entity->isOtherEnemy(localPlayer.get())) {
            continue;
        }

        beam_info.m_vecStart = position;
        beam_info.m_vecEnd = entity->getEyePosition();

        if (entity->isFlashed()) {
            beam_info.m_flRed =  255;
            beam_info.m_flGreen = 255;
            beam_info.m_flBlue = 255;
        }
        else if (entity->health() < 20) {
            beam_info.m_flRed = 255;
            beam_info.m_flGreen = 0;
            beam_info.m_flBlue = 0;
        }

        auto beam = memory->renderBeams->CreateBeamPoints(beam_info);


        if (beam)
            memory->renderBeams->DrawBeam(beam);
        
    }
    */
}


int Visuals::lastSize = 0;



void Visuals::BounceRing(GameEvent* event) noexcept
{
    if (!config->visuals.grenadeBounce.enabled || !interfaces->engine->isInGame() || !interfaces->engine->isConnected() || memory->renderBeams == nullptr)
        return;

    //const auto player = interfaces->entityList->getEntity(interfaces->engine->getPlayerForUserID(event->getInt("userid")));
    //const auto nade = interfaces->entityList->getEntity(event->getInt("entityid"));

    //std::vector<Vector> player_points;


    if (!localPlayer)
        return;

    Vector position;
    position.x = event->getFloat("x");
    position.y = event->getFloat("y");
    position.z = event->getFloat("z");

    BeamInfo_t beam_info;
    beam_info.m_nType = TE_BEAMRINGPOINT;
    beam_info.m_pszModelName = "sprites/purplelaser1.vmt";
    beam_info.m_nModelIndex = interfaces->modelInfo->getModelIndex("sprites/purplelaser1.vmt");
    beam_info.m_pszHaloName = "sprites/purplelaser1.vmt";
    beam_info.m_nHaloIndex = interfaces->modelInfo->getModelIndex("sprites/purplelaser1.vmt");
    beam_info.m_flHaloScale = 5;
    beam_info.m_flLife = 3.f;
    beam_info.m_flWidth = 15;
    beam_info.m_flFadeLength = 1.0f;
    beam_info.m_flAmplitude = 0.f;
    beam_info.m_flBrightness = 255;
    beam_info.m_flSpeed = 0;
    beam_info.m_nStartFrame = 0;
    beam_info.m_flFrameRate = 1;
    beam_info.m_nSegments = 1;
    beam_info.m_bRenderable = true;
    beam_info.m_nFlags = 0;
    beam_info.m_vecCenter = position;
    beam_info.m_flStartRadius = 1;
    beam_info.m_flEndRadius = 300;
    beam_info.m_flRed = config->visuals.grenadeBounce.color[0] * 255;
    beam_info.m_flGreen = config->visuals.grenadeBounce.color[1] * 255;
    beam_info.m_flBlue = config->visuals.grenadeBounce.color[2] * 255;

    // create beam backwards because it looks nicer.

    // create beam backwards because it looks nicer.
    beam_info.m_vecStart = position;

    auto beam = memory->renderBeams->CreateBeamRingPoint(beam_info);
    if (beam)
        memory->renderBeams->DrawBeam(beam);
}

#include "../Walkbot.h"
void Visuals::WalkbotRing() noexcept
{
    if (!interfaces->engine->isInGame() || !interfaces->engine->isConnected() || memory->renderBeams == nullptr)
        return;

    //const auto player = interfaces->entityList->getEntity(interfaces->engine->getPlayerForUserID(event->getInt("userid")));
    //const auto nade = interfaces->entityList->getEntity(event->getInt("entityid"));

    //std::vector<Vector> player_points;

    if (!localPlayer || Walkbot::curr_path.empty())
        return;

    if (Walkbot::curr_path.size() == lastSize)
        return;

    lastSize = Walkbot::curr_path.size();

    //if (true)
    //    return;


    Vector position;
    position.x = Walkbot::curr_path.front().toVector().x;
    position.y = Walkbot::curr_path.front().toVector().y;
    position.z = Walkbot::curr_path.front().toVector().z;

    BeamInfo_t beam_info;
    beam_info.m_nType = TE_BEAMRINGPOINT;
    beam_info.m_pszModelName = "sprites/purplelaser1.vmt";
    beam_info.m_nModelIndex = interfaces->modelInfo->getModelIndex("sprites/purplelaser1.vmt");
    beam_info.m_pszHaloName = "sprites/purplelaser1.vmt";
    beam_info.m_nHaloIndex = interfaces->modelInfo->getModelIndex("sprites/purplelaser1.vmt");
    beam_info.m_flHaloScale = 5;
    beam_info.m_flLife = 3.f;
    beam_info.m_flWidth = 15;
    beam_info.m_flFadeLength = 1.0f;
    beam_info.m_flAmplitude = 0.f;
    beam_info.m_flBrightness = 255;
    beam_info.m_flSpeed = 0;
    beam_info.m_nStartFrame = 0;
    beam_info.m_flFrameRate = 1;
    beam_info.m_nSegments = 1;
    beam_info.m_bRenderable = true;
    beam_info.m_nFlags = 0;
    beam_info.m_vecCenter = position;
    beam_info.m_flStartRadius = 1;
    beam_info.m_flEndRadius = 300;
    beam_info.m_flRed = config->visuals.grenadeBounce.color[0] * 255;
    beam_info.m_flGreen = config->visuals.grenadeBounce.color[1] * 255;
    beam_info.m_flBlue = config->visuals.grenadeBounce.color[2] * 255;

    // create beam backwards because it looks nicer.

    // create beam backwards because it looks nicer.
    beam_info.m_vecStart = position;

    auto beam = memory->renderBeams->CreateBeamRingPoint(beam_info);
    if (beam)
        memory->renderBeams->DrawBeam(beam);
}
/*
void Visuals::AimbotFov() noexcept {

}
*/
