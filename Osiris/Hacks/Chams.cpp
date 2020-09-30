#include <functional>

#include "Chams.h"
#include "../Config.h"
#include "Resolver.h"
#include "../Hooks.h"
#include "../Interfaces.h"
#include "Backtrack.h"
#include "../SDK/Entity.h"
#include "../SDK/EntityList.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/Material.h"
#include "../SDK/MaterialSystem.h"
#include "../SDK/StudioRender.h"
#include "../SDK/KeyValues.h"
#include "Animations.h"

Chams::Chams() noexcept
{
    normal = interfaces->materialSystem->createMaterial("normal", KeyValues::fromString("VertexLitGeneric", nullptr));
    flat = interfaces->materialSystem->createMaterial("flat", KeyValues::fromString("UnlitGeneric", nullptr));
    chrome = interfaces->materialSystem->createMaterial("chrome", KeyValues::fromString("VertexLitGeneric", "$envmap env_cubemap"));
    glow = interfaces->materialSystem->createMaterial("glow", KeyValues::fromString("VertexLitGeneric", "$additive 1 $envmap models/effects/cube_white $envmapfresnel 1 $alpha .8"));
    pearlescent = interfaces->materialSystem->createMaterial("pearlescent", KeyValues::fromString("VertexLitGeneric", "$ambientonly 1 $phong 1 $pearlescent 3 $basemapalphaphongmask 1"));

    metallic = interfaces->materialSystem->createMaterial("metallic", KeyValues::fromString("VertexLitGeneric", "$basetexture white $ignorez 0 $envmap env_cubemap  $envmapcontrast 1 $nofog 1 $model 1 $nocull 0 $selfillum 1 $halfambert 1 $znearer 0 $flat 1"));
    //metallic = interfaces->materialSystem->createMaterial("custom", KeyValues::fromString("VertexLitGeneric", "$basetexture white $ignorez 0 $envmap env_cubemap $normalmapalphaenvmapmask 1 $envmapcontrast 1 $nofog 0 $model 1 $nocull 0 $selfillum 1 $halfambert 0 $znearer 0 $flat 0"));

    metalsnow = interfaces->materialSystem->createMaterial("metalsnow", KeyValues::fromString("VertexLitGeneri", "$additive 1 $envmap models/effects/urban_puddle01a_normal"));
    glasswindow = interfaces->materialSystem->createMaterial("glasswindow", KeyValues::fromString("LightmappedGeneric", "$basetexture glass/hr_g/glass01_inferno $normalmapalphaenvmapmask 1"));
    c4_gift = interfaces->materialSystem->createMaterial("c4_gift", KeyValues::fromString("VertexLitGeneric", "$basetexture models/destruction_tanker/blackcable $normalmapalphaenvmapmask 1"));
    urban_puddle = interfaces->materialSystem->createMaterial("urban_puddle", KeyValues::fromString("VertexLitGeneric", "$basetexture models/effects/urban_puddle01a_normal"));
    crystal_cube = interfaces->materialSystem->createMaterial("crystal_cube", KeyValues::fromString("VertexLitGeneric", "$additive 1 $envmap models/effects/crystal_cube_vertigo_hdr $envmapfresnel 1"));
    ghost1 = interfaces->materialSystem->createMaterial("ghost1", KeyValues::fromString("VertexLitGeneric", "$basetexture models/seagull/seagull 1 $normalmapalphaenvmapmask 1"));

    zombie = interfaces->materialSystem->createMaterial("zombie", KeyValues::fromString("VertexLitGeneric", "$basetexture models/player/zombie/csgo_zombie_skin 1 $normalmapalphaenvmapmask 1"));
    searchlight = interfaces->materialSystem->createMaterial("searchlight", KeyValues::fromString("VertexLitGeneric", "$basetexture models/props_lighting/light_shop 1 $normalmapalphaenvmapmask 1"));
    brokenglass = interfaces->materialSystem->createMaterial("brokenglass", KeyValues::fromString("VertexLitGeneric", "$basetexture christmas/metal_roof_snow_1 $bumpmap models/inventory_items/hydra_crystal/hydra_crystal $envmapfresnel 1  $phong 1  $basemapalphaphongmask 1 $selfillum 1 $halfambert 1 $znearer 0"));

    crystal_blue = interfaces->materialSystem->createMaterial("crystal_blue", KeyValues::fromString("VertexLitGeneric", "$basetexture cs_office/cs_whiteboard_04 $bumpmap models/inventory_items/hydra_crystal/hydra_crystal $envmapfresnel 1 $envmap models/effects/crystal_cube_vertigo_hdr $phong 1 $pearlescent 3 $basemapalphaphongmask 1 $selfillum 1 $halfambert 1 $znearer 0"));
    velvet = interfaces->materialSystem->createMaterial("handrail", KeyValues::fromString("VertexLitGeneric", "$basetexture cs_assault/assault_handrails01 $bumpmap models/inventory_items/hydra_crystal/hydra_crystal $envmapfresnel 1  $phong 1  $basemapalphaphongmask 1"));
    metalwall = interfaces->materialSystem->createMaterial("metalwall", KeyValues::fromString("VertexLitGeneric", "$basetexture de_vertigo/tv_news01 $bumpmap models/inventory_items/hydra_crystal/hydra_crystal $envmapfresnel 1  $phong 1  $basemapalphaphongmask 1"));

    whiteboard01 = interfaces->materialSystem->createMaterial("whiteboard01", KeyValues::fromString("VertexLitGeneric", "$basetexture cs_office/cs_whiteboard_01 $bumpmap models/inventory_items/hydra_crystal/hydra_crystal $envmapfresnel 1  $phong 1  $basemapalphaphongmask 1"));
    whiteboard04 = interfaces->materialSystem->createMaterial("whiteboard01", KeyValues::fromString("VertexLitGeneric", "$basetexture cs_office/cs_whiteboard_04 $bumpmap models/inventory_items/hydra_crystal/hydra_crystal $envmapfresnel 1  $phong 1  $basemapalphaphongmask 1"));
    water = interfaces->materialSystem->createMaterial("water", KeyValues::fromString("VertexLitGeneric", "$basetexture cs_assault/windowbrightness $bumpmap models/inventory_items/hydra_crystal/hydra_crystal $envmapfresnel 1  $phong 1  $basemapalphaphongmask 1 $selfillum 1 $halfambert 1 $znearer 0"));
    
    /*
    $baseTexture black $bumpmap models/inventory_items/hydra_bronze/hydra_bronze $additive 1 $envmap editor/cube_vertigo $envmapfresnel 1 $normalmapalphaenvmapmask 1
    \

    models/inventory_items/hydra_crystal/hydra_crystal

    odels/inventory_items/hydra_bronze/hydra_bronze

    models/inventory_items/trophy_majors/crystal_blue
     models/inventory_items/trophy_majors/velvet
    
    */

    {
        const auto kv = KeyValues::fromString("VertexLitGeneric", "$envmap editor/cube_vertigo $envmapcontrast 1 $basetexture dev/zone_warning proxies { texturescroll { texturescrollvar $basetexturetransform texturescrollrate 0.6 texturescrollangle 90 } }");
        kv->setString("$envmaptint", "[.7 .7 .7]");
        animated = interfaces->materialSystem->createMaterial("animated", kv);
    }

    {
        const auto kv = KeyValues::fromString("VertexLitGeneric", "$baseTexture models/player/ct_fbi/ct_fbi_glass $envmap env_cubemap");
        kv->setString("$envmaptint", "[.4 .6 .7]");
        platinum = interfaces->materialSystem->createMaterial("platinum", kv);
    }

    {
        const auto kv = KeyValues::fromString("VertexLitGeneric", "$baseTexture detail/dt_metal1 $additive 1 $envmap editor/cube_vertigo");
        kv->setString("$color", "[.05 .05 .05]");
        glass = interfaces->materialSystem->createMaterial("glass", kv);
    }

    {
        const auto kv = KeyValues::fromString("VertexLitGeneric", "$baseTexture black $bumpmap effects/flat_normal $translucent 1 $envmap models/effects/crystal_cube_vertigo_hdr $envmapfresnel 0 $phong 1 $phongexponent 16 $phongboost 2");
        kv->setString("$phongtint", "[.2 .35 .6]");
        crystal = interfaces->materialSystem->createMaterial("crystal", kv);
    }

    {
        const auto kv = KeyValues::fromString("VertexLitGeneric", "$baseTexture white $bumpmap effects/flat_normal $envmap editor/cube_vertigo $envmapfresnel .6 $phong 1 $phongboost 2 $phongexponent 8");
        kv->setString("$color2", "[.05 .05 .05]");
        kv->setString("$envmaptint", "[.2 .2 .2]");
        kv->setString("$phongfresnelranges", "[.7 .8 1]");
        kv->setString("$phongtint", "[.8 .9 1]");
        silver = interfaces->materialSystem->createMaterial("silver", kv);
    }

    {
        const auto kv = KeyValues::fromString("VertexLitGeneric", "$baseTexture white $bumpmap effects/flat_normal $envmap editor/cube_vertigo $envmapfresnel .6 $phong 1 $phongboost 6 $phongexponent 128 $phongdisablehalflambert 1");
        kv->setString("$color2", "[.18 .15 .06]");
        kv->setString("$envmaptint", "[.6 .5 .2]");
        kv->setString("$phongfresnelranges", "[.7 .8 1]");
        kv->setString("$phongtint", "[.6 .5 .2]");
        gold = interfaces->materialSystem->createMaterial("gold", kv);
    }

    {
        const auto kv = KeyValues::fromString("VertexLitGeneric", "$baseTexture black $bumpmap models/inventory_items/trophy_majors/matte_metal_normal $additive 1 $envmap editor/cube_vertigo $envmapfresnel 1 $normalmapalphaenvmapmask 1 $phong 1 $phongboost 20 $phongexponent 3000 $phongdisablehalflambert 1");
        kv->setString("$phongfresnelranges", "[.1 .4 1]");
        kv->setString("$phongtint", "[.8 .9 1]");
        plastic = interfaces->materialSystem->createMaterial("plastic", kv);
    }
}

bool Chams::render(void* ctx, void* state, const ModelRenderInfo& info, matrix3x4* customBoneToWorld) noexcept
{
    appliedChams = false;
    this->ctx = ctx;
    this->state = state;
    this->info = &info;
    this->customBoneToWorld = customBoneToWorld;

    if (std::string_view{ info.model->name }.starts_with("models/weapons/v_")) {
        // info.model->name + 17 -> small optimization, skip "models/weapons/v_"
        if (std::strstr(info.model->name + 17, "sleeve"))
            renderSleeves();
        else if (std::strstr(info.model->name + 17, "arms"))
            renderHands();
        else if (!std::strstr(info.model->name + 17, "tablet")
            && !std::strstr(info.model->name + 17, "parachute")
            && !std::strstr(info.model->name + 17, "fists"))
            renderWeapons();
    } else {
        const auto entity = interfaces->entityList->getEntity(info.entityIndex);
        if (entity && !entity->isDormant() && entity->isPlayer()) {
            renderPlayer(entity);
        }
        else if (entity && (entity->isDormant() || !entity->isAlive()) && entity->isPlayer() && entity->isOtherEnemy(localPlayer.get()) && ((config->backtrack.enabled && config->backtrack.extendedrecords) || config->debug.animstatedebug.enabled)) {
            renderPlayer(entity, true);
        }

        std::string newStr = info.model->name;
        if (newStr.find("smoke") != std::string::npos) {
            renderSmoke();
        }

    }

    return appliedChams;
}

void Chams::renderPlayer(Entity* player, bool dead) noexcept
{
    if (!localPlayer)
        return;

    const auto health = player->health();

    if (const auto activeWeapon = player->getActiveWeapon(); activeWeapon && activeWeapon->getClientClass()->classId == ClassId::C4 && activeWeapon->c4StartedArming() && std::any_of(config->chams["Planting"].materials.cbegin(), config->chams["Planting"].materials.cend(), [](const Config::Chams::Material& mat) { return mat.enabled; })) {
        if (dead == false) {
            applyChams(config->chams["Planting"].materials, health);
        }
    } else if (player->isDefusing() && std::any_of(config->chams["Defusing"].materials.cbegin(), config->chams["Defusing"].materials.cend(), [](const Config::Chams::Material& mat) { return mat.enabled; })) {
        if (dead == false) {
            applyChams(config->chams["Defusing"].materials, health);
        }
    }
    else if (player == localPlayer.get()) {

        
        if (config->antiAim.enabled) {

            for (auto& i : Animations::data.fakematrix)
            {
                i[0][3] += info->origin.x;
                i[1][3] += info->origin.y;
                i[2][3] += info->origin.z;
            }

            auto mat = config->chams["Desync"].materials;
            if (localPlayer->isScoped()) {
                for (int b = 0; b < mat.size(); b++) {
                    auto m_material = &(mat.at(b));
                    m_material->color[3] = 0.4;
                }
            }
            applyChams(mat, health, Animations::data.fakematrix);

            for (auto& i : Animations::data.fakematrix)
            {
                i[0][3] -= info->origin.x;
                i[1][3] -= info->origin.y;
                i[2][3] -= info->origin.z;
            }
            auto lmat = config->chams["Local player"].materials;
            if (localPlayer->isScoped()) {
                for (int b = 0; b < lmat.size(); b++) {
                    auto m_material = &(lmat.at(b));
                    m_material->color[3] = 0.4;
                }
            }

            applyChams(lmat, health);
        }
        



    
        

    } else if (localPlayer->isOtherEnemy(player)) {


        if (!player->isDormant() || dead == true) {

            if (!config->debug.animstatedebug.resolver.enabled) {
                if (dead == false) {
                    applyChams(config->chams["Enemies"].materials, health);
                }
            }
            else {
                auto record = &Resolver::PlayerRecords[player->index()];
                if (!record || (record->invalid == true) || !record->wasTargeted) {
                    if (dead == false) {
                        applyChams(config->chams["Enemies"].materials, health);
                    }
                }
                else {
                    auto mat = config->chams["Enemies"].materials;
                    for (int b = 0; b < mat.size(); b++) {
                        auto m_material = &(mat.at(b));
                        m_material->color = { 1, 1, 1 };
                        m_material->color[3] = 1.0;
                    }
                    if (dead == false) {
                        applyChams(mat, health);
                    }

                    if (!record->shots.empty() && config->debug.showshots) {
                        auto mat = config->chams["Enemies"].materials;
                        for (auto shot : record->shots) {
                            for (int b = 0; b < mat.size(); b++) {
                                auto m_material = &(mat.at(b));
                                m_material->color = { 1, 0, 1 };
                                m_material->color[3] = 0.8f - (0.8f * ((memory->globalVars->currenttime - shot.simtime) / 4));
                            }

                            applyChams(mat, health, shot.matrix);
                        }
                    }

                }




            }
        }
        if (config->backtrack.enabled) {
            auto record = &Backtrack::records[player->index()];
            if (record && record->size() && Backtrack::valid(record->front().simulationTime)) {
                if (!player->isDormant()) {
                    if (!appliedChams)
                        hooks->modelRender.callOriginal<void, 21>(ctx, state, info, customBoneToWorld);
                    if (config->backtrack.backtrackAll) {
                        for (int i = 0; i < record->size(); i += config->backtrack.step) {
                            auto matrix = record->at(i).matrix;
                            if (!record->at(i).btTargeted) {
                                applyChams(config->chams["Backtrack"].materials, health, matrix);
                            }
                            else {
                                auto mat = config->chams["Backtrack"].materials;
                                for (int b = 0; b < mat.size(); b++) {
                                    auto m_material = &(mat.at(b));
                                    m_material->color = { .2, 1, .2 };
                                    m_material->color[3] = 1.0;

                                }
                                record->at(i).btTargeted = false;
                                applyChams(mat, health, matrix);
                            }

                        }
                    }
                    else {
                        applyChams(config->chams["Backtrack"].materials, health, record->back().matrix);
                    }
                }

                if (config->backtrack.extendedrecords) {
                    if (!appliedChams)
                        hooks->modelRender.callOriginal<void, 21>(ctx, state, info, customBoneToWorld);
                    auto ext_record = &Backtrack::extended_records[player->index()];
                    if (ext_record && !(ext_record->empty()) && (ext_record->size() >= 1) ) {
                        auto mat = config->chams["Backtrack"].materials;
                        for (int i = 0; i < ext_record->size(); i++) {
                            auto erecord = &(ext_record->at(i));
                            auto matrix = erecord->matrix; 
                            erecord->alpha -= (.020f * (config->backtrack.breadexisttime / 100)); //(.001 - .015)
                            if (erecord->alpha < 0) {
                                erecord->invalid = true;
                                continue;
                            }
                            for (int b = 0; b < mat.size(); b++) {
                                auto m_material = &(mat.at(b));
                                m_material->color[3] = erecord->alpha;
                            }
                            applyChams(mat, health, matrix);
                        }
                    }
                }

                interfaces->studioRender->forcedMaterialOverride(nullptr);
                
            }
        }

    } else {
        if (dead == false) {
            applyChams(config->chams["Allies"].materials, health);
        }
    }
}




void Chams::renderWeapons() noexcept
{
    if (!localPlayer || !localPlayer->isAlive() || localPlayer->isScoped())
        return;

    applyChams(config->chams["Weapons"].materials, localPlayer->health());
}

void Chams::renderHands() noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    applyChams(config->chams["Hands"].materials, localPlayer->health());
}

void Chams::renderSmoke() noexcept {
    if (!localPlayer || !localPlayer->isAlive())
        return;

    applyChams(config->chams["Hands"].materials, localPlayer->health());
}


void Chams::renderSleeves() noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    applyChams(config->chams["Sleeves"].materials, localPlayer->health());
}

void Chams::applyChams(const std::array<Config::Chams::Material, 7>& chams, int health, matrix3x4* customMatrix) noexcept
{
    for (const auto& cham : chams) {
        if (!cham.enabled || !cham.ignorez)
            continue;

        const auto material = dispatchMaterial(cham.material);
        if (!material)
            continue;
        
        float r, g, b;
        if (cham.healthBased && health) {
            r = 1.0f - health / 100.0f;
            g = health / 100.0f;
            b = 0.0f;
        } else if (cham.rainbow) {
            std::tie(r, g, b) = rainbowColor(cham.rainbowSpeed);
        } else {
            r = cham.color[0];
            g = cham.color[1];
            b = cham.color[2];
        }

        if (material == glow || material == chrome || material == plastic || material == glass || material == crystal)
            material->findVar("$envmaptint")->setVectorValue(r, g, b);
        else
            material->colorModulate(r, g, b);

        const auto pulse = cham.color[3] * (cham.blinking ? std::sin(memory->globalVars->currenttime * 5) * 0.5f + 0.5f : 1.0f);

        if (material == glow)
            material->findVar("$envmapfresnelminmaxexp")->setVecComponentValue(9.0f * (1.2f - pulse), 2);
        else
            material->alphaModulate(pulse);

        material->setMaterialVarFlag(MaterialVarFlag::IGNOREZ, true);
        material->setMaterialVarFlag(MaterialVarFlag::WIREFRAME, cham.wireframe);
        interfaces->studioRender->forcedMaterialOverride(material);
        hooks->modelRender.callOriginal<void, 21>(ctx, state, info, customMatrix ? customMatrix : customBoneToWorld);
        interfaces->studioRender->forcedMaterialOverride(nullptr);
    }

    for (const auto& cham : chams) {
        if (!cham.enabled || cham.ignorez)
            continue;

        const auto material = dispatchMaterial(cham.material);
        if (!material)
            continue;

        float r, g, b;
        if (cham.healthBased && health) {
            r = 1.0f - health / 100.0f;
            g = health / 100.0f;
            b = 0.0f;
        } else if (cham.rainbow) {
            std::tie(r, g, b) = rainbowColor(cham.rainbowSpeed);
        } else {
            r = cham.color[0];
            g = cham.color[1];
            b = cham.color[2];
        }

        if (material == glow || material == chrome || material == plastic || material == glass || material == crystal)
            material->findVar("$envmaptint")->setVectorValue(r, g, b);
        else
            material->colorModulate(r, g, b);

        const auto pulse = cham.color[3] * (cham.blinking ? std::sin(memory->globalVars->currenttime * 5) * 0.5f + 0.5f : 1.0f);

        if (material == glow)
            material->findVar("$envmapfresnelminmaxexp")->setVecComponentValue(9.0f * (1.2f - pulse), 2);
        else
            material->alphaModulate(pulse);

        if (cham.cover && !appliedChams)
            hooks->modelRender.callOriginal<void, 21>(ctx, state, info, customMatrix ? customMatrix : customBoneToWorld);

        material->setMaterialVarFlag(MaterialVarFlag::IGNOREZ, false);
        material->setMaterialVarFlag(MaterialVarFlag::WIREFRAME, cham.wireframe);
        interfaces->studioRender->forcedMaterialOverride(material);
        hooks->modelRender.callOriginal<void, 21>(ctx, state, info, customMatrix ? customMatrix : customBoneToWorld);
        appliedChams = true;
    }
}
