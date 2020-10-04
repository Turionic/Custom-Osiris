#include "Aimbot.h"

#include "Animations.h"
#include "../Config.h"
#include "../SDK/ConVar.h"
#include "../Interfaces.h"
#include "../Memory.h"
#include "../SDK/Entity.h"
#include "../SDK/UserCmd.h"
#include "../SDK/Vector.h"
#include "../SDK/WeaponId.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/PhysicsSurfaceProps.h"
#include "../SDK/WeaponData.h"
#include "../SDK/ModelInfo.h"
#include "../SDK/matrix3x4.h"
#include "../SDK/Math.h"
#include "../Multipoints.h"
#include "Resolver.h"
#include "AutoWall.h"
#include "Backtrack.h"
#include "../SDK/Angle.h"
//DEBUG
#include <fstream>
#include <iostream>
#include <cstddef>
#include <thread>
#include <future>

#include <math.h>

Vector Aimbot::calculateRelativeAngle(const Vector& source, const Vector& destination, const Vector& viewAngles) noexcept
{
    Vector delta = destination - source;
    Vector angles{ radiansToDegrees(atan2f(-delta.z, std::hypotf(delta.x, delta.y))) - viewAngles.x,
                   radiansToDegrees(atan2f(delta.y, delta.x)) - viewAngles.y };
    angles.normalize();
    return angles;
}

static float handleBulletPenetration(SurfaceData* enterSurfaceData, const Trace& enterTrace, const Vector& direction, Vector& result, float penetration, float damage) noexcept
{
    Vector end;
    Trace exitTrace;
    __asm {
        mov ecx, end
        mov edx, enterTrace
    }
    if (!memory->traceToExit(enterTrace.endpos.x, enterTrace.endpos.y, enterTrace.endpos.z, direction.x, direction.y, direction.z, exitTrace))
        return -1.0f;

    SurfaceData* exitSurfaceData = interfaces->physicsSurfaceProps->getSurfaceData(exitTrace.surface.surfaceProps);

    float damageModifier = 0.16f;
    float penetrationModifier = (enterSurfaceData->penetrationmodifier + exitSurfaceData->penetrationmodifier) / 2.0f;

    if (enterSurfaceData->material == 71 || enterSurfaceData->material == 89) {
        damageModifier = 0.05f;
        penetrationModifier = 3.0f;
    } else if (enterTrace.contents >> 3 & 1 || enterTrace.surface.flags >> 7 & 1) {
        penetrationModifier = 1.0f;
    }

    if (enterSurfaceData->material == exitSurfaceData->material) {
        if (exitSurfaceData->material == 85 || exitSurfaceData->material == 87)
            penetrationModifier = 3.0f;
        else if (exitSurfaceData->material == 76)
            penetrationModifier = 2.0f;
    }

    damage -= 11.25f / penetration / penetrationModifier + damage * damageModifier + (exitTrace.endpos - enterTrace.endpos).squareLength() / 24.0f / penetrationModifier;

    result = exitTrace.endpos;
    return damage;
}

static bool canScan(Entity* entity, const Vector& destination, const WeaponInfo* weaponData, int minDamage, bool allowFriendlyFire, float& damageret) noexcept
{
    if (!localPlayer)
        return false;

    float damage{ static_cast<float>(weaponData->damage) };

    Vector start{ localPlayer->getEyePosition() };
    Vector direction{ destination - start };
    direction /= direction.length();

    int hitsLeft = 4;

    while (damage >= 1.0f && hitsLeft) {
        Trace trace;
        interfaces->engineTrace->traceRay({ start, destination }, 0x4600400B, localPlayer.get(), trace);

        if (!allowFriendlyFire && trace.entity && trace.entity->isPlayer() && !localPlayer->isOtherEnemy(trace.entity))
            return false;

        if (trace.fraction == 1.0f)
            break;

        if (trace.entity == entity && trace.hitgroup > HitGroup::Generic && trace.hitgroup <= HitGroup::RightLeg) {
            damage = HitGroup::getDamageMultiplier(trace.hitgroup) * damage * powf(weaponData->rangeModifier, trace.fraction * weaponData->range / 500.0f);

            if (float armorRatio{ weaponData->armorRatio / 2.0f }; HitGroup::isArmored(trace.hitgroup, trace.entity->hasHelmet()))
                damage -= (trace.entity->armor() < damage * armorRatio / 2.0f ? trace.entity->armor() * 4.0f : damage) * (1.0f - armorRatio);

            damageret = damage;
            return (damage >= minDamage) || (damage >= entity->health());


        }
        const auto surfaceData = interfaces->physicsSurfaceProps->getSurfaceData(trace.surface.surfaceProps);

        if (0.1f > surfaceData->penetrationmodifier )
            break;

        damage = handleBulletPenetration(surfaceData, trace, direction, start, weaponData->penetration, damage);

        damageret = damage;

        hitsLeft--;
    }
    return false;
}

static void setRandomSeed(int seed) noexcept
{
    using randomSeedFn = void(*)(int);
    static auto randomSeed{ reinterpret_cast<randomSeedFn>(GetProcAddress(GetModuleHandleA("vstdlib.dll"), "RandomSeed")) };
    randomSeed(seed);
}

static float getRandom(float min, float max) noexcept
{
    using randomFloatFn = float(*)(float, float);
    static auto randomFloat{ reinterpret_cast<randomFloatFn>(GetProcAddress(GetModuleHandleA("vstdlib.dll"), "RandomFloat")) };
    return randomFloat(min, max);
}

#include "../Debug.h"
static float Aimbot::hitChance(Entity* localPlayer, Entity* entity, Entity* weaponData, const Vector& destination, const UserCmd* cmd, const int hitChance) noexcept
{

    if (!hitChance)
        return 100.0f;

    Debug::LogItem item;
    item.PrintToConsole = true;
    
    int maxSeed = 256;

    const Angle angles(destination + cmd->viewangles);

    int hits = 0;
    const int hitsNeed = static_cast<int>(static_cast<float>(maxSeed) * (static_cast<float>(hitChance) / 100.f));

    const auto weapSpread = weaponData->getSpread();
    const auto weapInaccuracy = weaponData->getInaccuracy();
    const auto localEyePosition = localPlayer->getEyePosition();
    const auto range = weaponData->getWeaponData()->range;

    for (int i = 0; i < maxSeed; ++i)
    {
        setRandomSeed(i + 1);
        float inaccuracy = getRandom(0.f, 1.f);
        float spread = getRandom(0.f, 1.f);
        const float spreadX = getRandom(0.f, 2.f * static_cast<float>(M_PI));
        const float spreadY = getRandom(0.f, 2.f * static_cast<float>(M_PI));

        const auto weaponIndex = weaponData->itemDefinitionIndex2();
        const auto recoilIndex = weaponData->recoilIndex();
        if (weaponIndex == WeaponId::Revolver)
        {
            if (cmd->buttons & UserCmd::IN_ATTACK2)
            {
                inaccuracy = 1.f - inaccuracy * inaccuracy;
                spread = 1.f - spread * spread;
            }
        }
        else if (weaponIndex == WeaponId::Negev && recoilIndex < 3.f)
        {
            for (int i = 3; i > recoilIndex; --i)
            {
                inaccuracy *= inaccuracy;
                spread *= spread;
            }

            inaccuracy = 1.f - inaccuracy;
            spread = 1.f - spread;
        }

        inaccuracy *= weapInaccuracy;
        spread *= weapSpread;

        Vector spreadView{ (cosf(spreadX) * inaccuracy) + (cosf(spreadY) * spread),
                           (sinf(spreadX) * inaccuracy) + (sinf(spreadY) * spread) };
        Vector direction{ (angles.forward + (angles.right * spreadView.x) + (angles.up * spreadView.y)) * range };

        static Trace trace;
        interfaces->engineTrace->clipRayToEntity({ localEyePosition, localEyePosition + direction }, 0x4600400B, entity, trace);
        if (trace.entity == entity)
            ++hits;



        if (hits >= hitsNeed) {
            if (config->debug.aimbotcoutdebug) {
                item.PrintToScreen = false;
                item.text.push_back(L"Hitchance Completed With " + std::to_wstring((hits / hitsNeed) * 100.f) + L" Likelihood of hitting");
                Debug::LOG_OUT.push_front(item);
            }
            return (hits / hitsNeed) * 100.f;
        }

        if ((maxSeed - i + hits) < hitsNeed) {
            if (config->debug.aimbotcoutdebug) {
                item.PrintToScreen = false;
                item.text.push_back(L"Hitchance Completed With " + std::to_wstring((hits / hitsNeed) * 100.f) + L" Likelihood of hitting");
                Debug::LOG_OUT.push_front(item);
            }
            return -1.0f;
        }
    }           
    if (config->debug.aimbotcoutdebug) {
        item.PrintToScreen = false;
        item.text.push_back(L"Hitchance Completed With " + std::to_wstring((hits / hitsNeed) * 100.f) + L" Likelihood of hitting");
        Debug::LOG_OUT.push_front(item);
    }
    return -1.0f;
}




void Aimbot::Autostop(UserCmd* cmd) noexcept
{

    if (!localPlayer || !localPlayer->isAlive())
        return;

    Vector Velocity = localPlayer->velocity();

    if (Velocity.length2D() == 0)
        return;

    static float Speed = 450.f;

    Vector Direction;
    Vector RealView;
    Math::VectorAngles(Velocity, Direction);
    interfaces->engine->getViewAngles(RealView);
    Direction.y = RealView.y - Direction.y;

    Vector Forward;
    Math::AngleVectors(Direction, &Forward);
    Vector NegativeDirection = Forward * -Speed;

    cmd->forwardmove = NegativeDirection.x;
    cmd->sidemove = NegativeDirection.y;
}

/*
void Aimbot::run(UserCmd* cmd) noexcept
{
    if (!localPlayer || localPlayer->nextAttack() > memory->globalVars->serverTime())
        return;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon || !activeWeapon->clip())
        return;

    auto weaponIndex = getWeaponIndex(activeWeapon->itemDefinitionIndex2());
    if (!weaponIndex)
        return;

    auto weaponClass = getWeaponClass(activeWeapon->itemDefinitionIndex2());
    if (!config->aimbot[weaponIndex].enabled)
        weaponIndex = weaponClass;

    if (!config->aimbot[weaponIndex].enabled)
        weaponIndex = 0;

    if (!config->aimbot[weaponIndex].betweenShots && activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime())
        return;

    if (!config->aimbot[weaponIndex].ignoreFlash && localPlayer->isFlashed())
        return;

    if (config->aimbot[weaponIndex].onKey) {
        if (!config->aimbot[weaponIndex].keyMode) {
            if (!GetAsyncKeyState(config->aimbot[weaponIndex].key))
                return;
        }
        else {
            static bool toggle = true;
            if (GetAsyncKeyState(config->aimbot[weaponIndex].key) & 1)
                toggle = !toggle;
            if (!toggle)
                return;
        }
    }

    if (activeWeapon->isKnife() || activeWeapon->isGrenade())
        return;

    Entity* Target{};
    Vector AimPoint{};




    if (config->aimbot[weaponIndex].enabled && (cmd->buttons & UserCmd::IN_ATTACK || config->aimbot[weaponIndex].autoShot || config->aimbot[weaponIndex].aimlock) && activeWeapon->getInaccuracy() <= config->aimbot[weaponIndex].maxAimInaccuracy) {}





}

*/


void Aimbot::oldstyle(UserCmd* cmd) noexcept
{
    if (!localPlayer || localPlayer->nextAttack() > memory->globalVars->serverTime())
        return;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon || !activeWeapon->clip())
        return;

    auto weaponIndex = getWeaponIndex(activeWeapon->itemDefinitionIndex2());
    if (!weaponIndex)
        return;

    auto weaponClass = getWeaponClass(activeWeapon->itemDefinitionIndex2());
    if (!config->aimbot[weaponIndex].enabled)
        weaponIndex = weaponClass;

    if (!config->aimbot[weaponIndex].enabled)
        weaponIndex = 0;

    if (!config->aimbot[weaponIndex].betweenShots && activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime())
        return;

    if (!config->aimbot[weaponIndex].betweenShots && activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime())
        return;

    if (!config->aimbot[weaponIndex].ignoreFlash && localPlayer->isFlashed())
        return;

    if (config->aimbot[weaponIndex].onKey) {
        if (!config->aimbot[weaponIndex].keyMode) {
            if (!GetAsyncKeyState(config->aimbot[weaponIndex].key))
                return;
        }
        else {
            static bool toggle = true;
            if (GetAsyncKeyState(config->aimbot[weaponIndex].key) & 1)
                toggle = !toggle;
            if (!toggle)
                return;
        }
    }


    if (config->aimbot[weaponIndex].enabled && (cmd->buttons & UserCmd::IN_ATTACK || config->aimbot[weaponIndex].autoShot || config->aimbot[weaponIndex].aimlock) && activeWeapon->getInaccuracy() <= config->aimbot[weaponIndex].maxAimInaccuracy) {

        if (config->aimbot[weaponIndex].scopedOnly && activeWeapon->isSniperRifle() && !localPlayer->isScoped())
            return;

        auto bestFov = config->aimbot[weaponIndex].fov;
        Vector bestTarget{ };
        auto localPlayerEyePosition = localPlayer->getEyePosition();

        const auto aimPunch = activeWeapon->requiresRecoilControl() ? localPlayer->getAimPunch() : Vector{ };



        Entity* ent_save = Resolver::TargetedEntity;
        if (!ent_save || ent_save->isDormant() || !ent_save->isAlive()) {
            ent_save = localPlayer.get();
            Resolver::TargetedEntity = localPlayer.get();
        }

        for (int i = 1; i <= interfaces->engine->getMaxClients(); i++) {
            Entity* entity;
            if (Resolver::TargetedEntity && (Resolver::TargetedEntity != localPlayer.get()))
                entity = Resolver::TargetedEntity;
            else
                entity = interfaces->entityList->getEntity(i);
            
            if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive()
                || !entity->isOtherEnemy(localPlayer.get()) && !config->aimbot[weaponIndex].friendlyFire || entity->gunGameImmunity()) {
                //Resolver::TargetedEntity = localPlayer.get();
                continue;
            }

            auto boneList = config->aimbot[weaponIndex].bone == 1 ? std::initializer_list{ 8, 4, 3, 7, 6, 5 } : std::initializer_list{ 8, 7, 6, 5, 4, 3 };

            if (!config->aimbot[weaponIndex].multipointenabled) {

                float damageret = 0;
                float maxDamage = 0;
                for (auto bone : boneList) {
                    auto bonePosition = entity->getBonePosition(config->aimbot[weaponIndex].bone > 1 ? 10 - config->aimbot[weaponIndex].bone : bone);





                    if (!entity->isVisible(bonePosition) && (config->aimbot[weaponIndex].visibleOnly || !canScan(entity, bonePosition, activeWeapon->getWeaponData(), config->aimbot[weaponIndex].killshot ? entity->health() : config->aimbot[weaponIndex].minDamage, config->aimbot[weaponIndex].friendlyFire, damageret)))
                        continue;

                    auto angle = calculateRelativeAngle(localPlayerEyePosition, bonePosition, cmd->viewangles + aimPunch);



                    auto fov = std::hypotf(angle.x, angle.y);
                    if (fov < bestFov) {
                        bestFov = fov;
                        bestTarget = bonePosition;
                        ent_save = entity;
                    }
                    if (config->aimbot[weaponIndex].bone) {
                        break;
                    }
                }
            }
            else {

                //std::ofstream myfile;
                //myfile.open("C:\\Users\\userWIN\\source\\repos\\test\\Debug\\aimbotlog.txt", std::ios_base::app);

                    // myfile << "Get Model Function called\n";

                const auto model = entity->getModel();
                if (!model) {
                    //myfile << "Get Model returned null model, exiting\n";
                    return;
                }
                //myfile << "Calling getStudioModel\n";
                const auto studioModel = interfaces->modelInfo->getStudioModel(model);
                if (!studioModel) {
                    //myfile << "getStudioModel retyrbed null model\n";
                    return;
                }

                // StudioHitboxSet* getHitboxSet(int i)
                // myfile << "Calling getHitboxSet\n";
                const auto studiohitboxset = studioModel->getHitboxSet(0);
                if (!studiohitboxset) {
                    // myfile << "getHitboxSet returned null \n";
                    return;
                }

                //if (!(matrix3x4 boneMatrices[256]; entity->setupBones(boneMatrices, 256, 256, 0.0f)))
                //   return;

                // Multipoint StudioHitBoxCode 
                float maxDamage = 0;
                float damage = 0;

                for (auto bone : boneList) {

                    //myfile << "bone = " << bone << "\n";
                    auto studiobone = studiohitboxset->getHitbox(config->aimbot[weaponIndex].bone > 1 ? 10 - config->aimbot[weaponIndex].bone : bone);

                    if (!studiobone) {
                        //myfile << "No Studiobone returned from getHitBox\n";
                        return;
                    }

                    //myfile << "studiobone->bone gives " << studiobone->bone;


                    matrix3x4 boneMatrices[256];
                    entity->setupBones(boneMatrices, 256, 256, 0.0f);

                    auto basebonePosition = entity->getBonePosition(studiobone->bone);

                    //myfile << "\nbasebonePosition = " << basebonePosition.x << " " << basebonePosition.y << " " << basebonePosition.z;



                   // myfile << "\nstudio->bbMin " << studiobone->bbMin.x << " " << studiobone->bbMin.y <<  " " << studiobone->bbMin.z;
                    //myfile << "\nstudio->bbMax " << studiobone->bbMax.x << " " << studiobone->bbMax.y << " " << studiobone->bbMax.z;

                    Vector bbmin = studiobone->bbMin * config->aimbot[weaponIndex].multidistance;
                    Vector bbmax = studiobone->bbMax * config->aimbot[weaponIndex].multidistance;

                    Vector points[] = { Vector{bbmin.x, bbmin.y, bbmin.z},
                         Vector{bbmin.x, bbmax.y, bbmin.z},
                         Vector{bbmax.x, bbmax.y, bbmin.z},
                         Vector{bbmax.x, bbmin.y, bbmin.z},
                         Vector{bbmax.x, bbmax.y, bbmax.z},
                         Vector{bbmin.x, bbmax.y, bbmax.z},
                         Vector{bbmin.x, bbmin.y, bbmax.z},
                         Vector{bbmax.x, bbmin.y, bbmax.z},
                    };

                    //auto bonePosition = entity->getBonePosition(config->aimbot[weaponIndex].bone > 1 ? 10 - config->aimbot[weaponIndex].bone : bone);
                    Vector bonePositions[8];
                    bonePositions[0] = basebonePosition;
                    int i = 1;
                    for (auto point : points) {
                        bonePositions[i] = point.transform(boneMatrices[bone]);
                        //myfile << "bonePosition: " << i << " is " << bonePositions[i].x << " " << bonePositions[i].y << " " << bonePositions->z << "\n";
                        i++;
                    }

                    //Vector bonePositions[] = { (basebonePosition - (studiobone->bbMin * config->aimbot[weaponIndex].multidistance)), basebonePosition, (basebonePosition + (studiobone->bbMax * config->aimbot[weaponIndex].multidistance)) };





                    for (auto bonePosition : bonePositions) {

                        // myfile << "\nCurrent Bone Position in multipoint is: " << bonePosition.x << " " << bonePosition.y << " " << bonePosition.z << "\n";
                        if (!entity->isVisible(bonePosition) && (config->aimbot[weaponIndex].visibleOnly || !canScan(entity, bonePosition, activeWeapon->getWeaponData(), config->aimbot[weaponIndex].killshot ? entity->health() : config->aimbot[weaponIndex].minDamage, config->aimbot[weaponIndex].friendlyFire, damage)))
                            continue;

                        auto angle = calculateRelativeAngle(localPlayerEyePosition, bonePosition, cmd->viewangles + aimPunch);
                        auto fov = std::hypotf(angle.x, angle.y);
                        if (fov < bestFov) {
                            bestFov = fov;
                            if ((damage > maxDamage)) {
                                bestTarget = bonePosition;
                                maxDamage = damage;
                                ent_save = entity;

                            }

                            if ((damage > 100.0f) || (entity->health() - damage <= 0)) {
                                bestTarget = bonePosition;
                                maxDamage = damage;
                                ent_save = entity;
                                break;
                            }

                        }
                        // myfile << "Curr Damage is: " << damage << " best damage is: " << maxDamage << "\n";


                    }

                    //myfile << "Best target is " << bestTarget.x << " " << bestTarget.y << " " << bestTarget.z << "\n";
                   // myfile.close();
                    if (config->aimbot[weaponIndex].bone || (damage > 100.0f) || (damage > entity->health()))
                        break;


                }









            }
        }

        

        if (config->misc.walkbot && (ent_save != localPlayer.get())) {
            Resolver::TargetedEntity = ent_save;
        }


        if (bestTarget.notNull() && (config->aimbot[weaponIndex].ignoreSmoke
            || !memory->lineGoesThroughSmoke(localPlayer->getEyePosition(), bestTarget, 1))) {

            if (config->misc.walkbot) {
                Resolver::TargetedEntity = ent_save;
                Autostop(cmd);
            }

            static Vector lastAngles{ cmd->viewangles };
            static int lastCommand{ };

            if (lastCommand == cmd->commandNumber - 1 && lastAngles.notNull() && config->aimbot[weaponIndex].silent)
                cmd->viewangles = lastAngles;

            auto angle = calculateRelativeAngle(localPlayer->getEyePosition(), bestTarget, cmd->viewangles + aimPunch);
            bool clamped{ false };

            if (fabs(angle.x) > config->misc.maxAngleDelta || fabs(angle.y) > config->misc.maxAngleDelta) {
                angle.x = std::clamp(angle.x, -config->misc.maxAngleDelta, config->misc.maxAngleDelta);
                angle.y = std::clamp(angle.y, -config->misc.maxAngleDelta, config->misc.maxAngleDelta);
                clamped = true;
            }

            PlayerInfo playerInfo; 
            interfaces->engine->getPlayerInfo(ent_save->index(), playerInfo);



            angle /= ((playerInfo.fakeplayer) && config->misc.walkbot)? 2.7 : ((bestFov > 130) ? config->aimbot[weaponIndex].smooth + 12 : config->aimbot[weaponIndex].smooth); // If its a bot, we FUCK EM UP!
            cmd->viewangles += angle;
            if (!config->aimbot[weaponIndex].silent)
                interfaces->engine->setViewAngles(cmd->viewangles);

            if (config->aimbot[weaponIndex].autoScope && activeWeapon->nextPrimaryAttack() <= memory->globalVars->serverTime() && activeWeapon->isSniperRifle() && !localPlayer->isScoped())
                cmd->buttons |= UserCmd::IN_ATTACK2;

     
            if (config->aimbot[weaponIndex].autoShot && activeWeapon->nextPrimaryAttack() <= memory->globalVars->serverTime() && !clamped && activeWeapon->getInaccuracy() <= config->aimbot[weaponIndex].maxShotInaccuracy) {
                cmd->buttons |= UserCmd::IN_ATTACK;
            }
            if (clamped)
                cmd->buttons &= ~UserCmd::IN_ATTACK;

            if (clamped || config->aimbot[weaponIndex].smooth > 1.0f) lastAngles = cmd->viewangles;
            else lastAngles = Vector{ };

            lastCommand = cmd->commandNumber;
        }
    }
}






// retrieveOne(Entity* entity, float multiPointsExpansion, Vector(&multiPoints)[Multipoints::MULTIPOINTS_MAX], int desiredHitBox)
// bool retrieveAll(Entity * entity, float multiPointsExpansion, Vector(&multiPoints)[Multipoints::HITBOX_MAX][Multipoints::MULTIPOINTS_MAX])
inline float DistanceToEntity(Entity* Enemy)
{
    return sqrt(pow(localPlayer->origin().x - Enemy->origin().x, 2) + pow(localPlayer->origin().y - Enemy->origin().y, 2) + pow(localPlayer->origin().z - Enemy->origin().z, 2));
}
/*
float Aimbot::willPointWork(Entity* entity, int weaponIndex, Vector AimPoint) {

    float bestDamage = 0;
    float damage = 0;
    Vector vAimPoint = AimPoint;
    damage = Autowall->Damage(vAimPoint);

    if (damage > entity->health()) {
            return damage;
    }
    else if ((damage > config->aimbot[weaponIndex].minDamage) && (!config->aimbot[weaponIndex].killshot)) { // if nearest it set, exit on first bone that fufills requirements
            return damage;
    }

    return 0;
}
*/
#include "StreamProofESP.h"
#include "../GameData.h"
#include "../imgui/imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../imgui/imgui_internal.h" 


static float worldToScreen(const Vector& in) noexcept
{
    const auto& matrix = GameData::toScreenMatrix();

    const auto w = matrix._41 * in.x + matrix._42 * in.y + matrix._43 * in.z + matrix._44;



    auto out = StreamProofESP::ScreenSize / 2.0f;
    auto tester = StreamProofESP::ScreenSize / 2.0f;
    out.x *= 1.0f + (matrix._11 * in.x + matrix._12 * in.y + matrix._13 * in.z + matrix._14) / w;
    out.y *= 1.0f - (matrix._21 * in.x + matrix._22 * in.y + matrix._23 * in.z + matrix._24) / w;

    if (w < 0.001f) {
        return 99999.0f;
    }

    return (std::abs(out.x - tester.x) + std::abs(out.y - tester.y)) / 2;
}


#include "../Debug.h"

#include <mutex>
#include <numeric>
#include <sstream>
#include <codecvt>
#include <locale>

void Aimbot::run(UserCmd* cmd, int& bestDamage_save, int& bestHitchance, Vector& wallbangVector) noexcept {

    // Does local player exists
    if (!localPlayer || localPlayer->nextAttack() > memory->globalVars->serverTime() || localPlayer->isDormant() || !localPlayer->isAlive())
        return;

    //Get the active weapon
    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon || !activeWeapon->clip()) // does active weapon exist and have a clip in
        return;

    auto weaponIndex = getWeaponIndex(activeWeapon->itemDefinitionIndex2()); // get weapon index
    if (!weaponIndex) // no weapon index? then exit
        return;

    // get weapon class
    auto weaponClass = getWeaponClass(activeWeapon->itemDefinitionIndex2());
    if (!config->aimbot[weaponIndex].enabled) //if weaponIndex is not enabled
        weaponIndex = weaponClass; // weaponClass is now weaponIndex

    if (!config->aimbot[weaponIndex].enabled) // If weaponClass isnt set
        weaponIndex = 0; // weaponClass/Index is now 0 (ALL setting)


    // BEYOND HERE REALLY AINT NEEDED UNLESS YOU WANNA BE PERFECT ABOUT WHEN ITS ON OR NOT
    if (!config->aimbot[weaponIndex].betweenShots && (activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime())) {
        return;
    }

    if (!config->aimbot[weaponIndex].ignoreFlash && localPlayer->isFlashed()) {
        return;
    }

    if (activeWeapon->getInaccuracy() >= config->aimbot[weaponIndex].maxTriggerInaccuracy)
    {
        oldstyle(cmd);
        return;
    }

    if (config->aimbot[weaponIndex].oldstyle) {
        oldstyle(cmd);
        return;
    }

    if (activeWeapon->isKnife() || activeWeapon->isGrenade()) {
        return;
    }

    if (!config->aimbot[weaponIndex].autoShot && !(cmd->buttons & UserCmd::IN_ATTACK) && !(config->aimbot[weaponIndex].aimlock)) {
        return;
    }
        

    struct EntityVal {
        Entity* entity_ptr;
        int entity = 0;
        float distance = 0;
        int health = 0;
        bool isVisible = false;
        bool wasTargeted = false;
        Resolver::Record* record = &Resolver::invalid_record;
        bool bt = false;
        bool onshot = false;
        float prevVelocity = 999.0f;
        bool lbyUpdated = false;
    };
    std::vector<EntityVal> enemies;

    //Debug::LogItem item;
    

    struct entity_sort {
        bool operator() (EntityVal A, EntityVal B) {

            if ((A.distance < 20.0f) && (B.distance > 20.0f)) {
                return true;
            }
            if ((A.distance > 20.0f) && (B.distance < 20.0f)) {
                return false;
            }
            if (A.isVisible && !B.isVisible) {
                return true;
            }
            else if (!A.isVisible && B.isVisible) {
                return false;
            }
            if (A.wasTargeted && !B.wasTargeted) {
                return true;
            }
            else if (!A.wasTargeted && B.wasTargeted) {
                return false;
            }

            else {
                return false;
            }
            if (A.entity < B.entity) { // WTF IS THIS
                return true;
            }

        }
    } ent_sort;


    static auto frameRate = 1.0f;
    frameRate = 1 / (0.9f * frameRate + 0.1f * memory->globalVars->absoluteFrameTime);


    bool visFound = false;
    for (int i = 1; i <= interfaces->engine->getMaxClients(); i++) {
        auto entity = interfaces->entityList->getEntity(i);
        if (!entity || (entity == localPlayer.get()) || entity->isDormant() || !entity->isAlive()
            || (!entity->isOtherEnemy(localPlayer.get()) && !config->aimbot[weaponIndex].friendlyFire) || entity->gunGameImmunity()) {
            Resolver::PlayerRecords.at(i).wasTargeted = false;
            continue;
        }
        EntityVal curr_ent;
        //curr_ent.distance = DistanceToEntity(entity);
        curr_ent.distance = worldToScreen(entity->getEyePosition());
        curr_ent.entity = entity->index();
        curr_ent.health = entity->health();
        curr_ent.entity_ptr = entity;
        Resolver::Record* record = &Resolver::PlayerRecords.at(entity->index());
        if (record && !record->invalid) {
            curr_ent.wasTargeted = record->wasTargeted;
            curr_ent.record = record;
            curr_ent.prevVelocity = record->prevVelocity;
        }
        else {
            curr_ent.wasTargeted = false;
            curr_ent.record = &Resolver::invalid_record;
        }

        if (!visFound) {
            curr_ent.isVisible = entity->isVisible();
            visFound = true;
        }
        else {
            curr_ent.isVisible = false;
        }
        /*
        if (config->optimization.nearestonly.enabled && (frameRate < config->optimzation.nearestonly.frames)) {
            if (!(curr_ent.isVisible) && !(curr_ent.wasTargeted) curr_ent.distance < 2000) {
                continue;
            }
        }
        */

        enemies.push_back(curr_ent);
    } // set up entities

    if (enemies.empty()) {
        return;
    }

    
    if (enemies.size() > 1) {
        std::sort(enemies.begin(), enemies.end(), ent_sort);
    }
    
    Resolver::Record* record = &Resolver::invalid_record;

    if (!enemies.front().wasTargeted) {
        enemies.front().wasTargeted = true;
    }
    //else if (enemies.size() > 2){
    //    enemies.at(1).wasTargeted = true;
    //}

    // config->debug.TraceLimit limits the amount of entities in the vector to 3 if under 20fps, hopefully saving your frames
    if ((20 > frameRate) && config->debug.TraceLimit && (enemies.size() > 3)) {
        enemies.erase((enemies.begin() + 2), enemies.end());
    }

    //if (enemies.size() > 3 && enemies.front().wasTargeted && enemies.front().record->FiredUpon) {
    //    enemies.erase((enemies.begin() + 1), enemies.end());
    //}




    // math time
    float bestDamage = -1.0f;
    Vector firstWallToPen;
    std::vector<VectorAndDamage> AimPoints;
    Entity* Target = localPlayer.get();//interfaces->entityList->getEntity(localPlayer->index());
    Vector BestPoint;
    Backtrack::Record* backup_rec;
    EntityVal Save; // Replace Entity* Target with this!
    for (auto enemy : enemies) {


        record = enemy.record;
        // Get ALL the multipoints
        float multiplier = record->multiExpan;


        if (config->debug.movefix) {
            Animations::setupMoveFix(enemy.entity_ptr);
        }
        /*

            IIIIIIIIIIIIIIIIII
            I                I
            I       h  .3    I
            I                I
            IIIIIIIIIIIIIIIIII <- 2.0
                           1.9


        */
        auto brecord = &Backtrack::records[enemy.entity]; // Get Backtrack Record
        if (!brecord->empty() && (brecord->size() > 1) && Backtrack::valid(brecord->front().simulationTime) && (enemy.wasTargeted /*|| Backtrack::ImportantTick(*brecord)*/)) { // Sanity // 

            bool ImportantTick = Backtrack::ImportantTick(*brecord);
            if ((!enemy.isVisible || ImportantTick || (enemy.entity_ptr->velocity().length2D() > 170.0f)) && config->backtrack.enabled) { // if Enemy isn't visible, or their velocity is  > 100, we go for bt
                for (int i = (brecord->front().onshot || brecord->front().lbyUpdated) ? 0 : 3; i < (brecord->size() - 1); i++) { // Ignore the last 2 records, i've noticed the likely hood of hitting those is essentially nonexistant
                    if (i >= brecord->size()) {
                        break;
                    }
                    Backtrack::Record* c_record = &(brecord->at(i));
                    if (!c_record || !Backtrack::valid(c_record->simulationTime)) {
                        continue;
                    }
                    else {
                        if ((localPlayer->canSeePoint(c_record->head)) || (c_record->onshot && config->aimbot[weaponIndex].onshot) || c_record->lbyUpdated) { // Visible backtrack, onshot, or lbyUpdated? Lets go for it!
                            if ((enemy.onshot && config->aimbot[weaponIndex].onshot) || enemy.lbyUpdated) { /* If it's an onshot or lbyUpdated Tick, we set override*/
                                Backtrack::SetOverride(enemy.entity_ptr, *c_record);
                            }
                            else {
                                if (ImportantTick && config->aimbot[weaponIndex].prioritizeEventBT) {
                                    continue;
                                }
                            }
                            enemy.bt = true;
                            c_record->btTargeted = true;
                            enemy.onshot = c_record->onshot;
                            c_record->onshot = false;
                            enemy.lbyUpdated = c_record->lbyUpdated;
                            c_record->lbyUpdated = false;
                            Animations::setup(enemy.entity_ptr, *c_record);
                            backup_rec = c_record;
                            break;
                        }
                        else {
                            //Animations::finishSetup(enemy.entity_ptr);
                            continue;
                        }

                    }

                }
            }
        } /* For Months I spent pissed off my shit would not shoot and always got insta tapped.... had this code AFTER I got the multipoints. Fuck my life up dawg */

        /* If the enemy hasnt moved, we move the points in from *2.0 to slightly below multidistance*/
        if (enemy.wasTargeted) {
            if (/*(enemy.entity_ptr->velocity().length2D() < 1.01) && (enemy.record->prevVelocity < 1.01) &&*/ config->aimbot[weaponIndex].dynamicpoints) {
                if (enemy.record->prevhealth == enemy.health) {
                    if (record->multiExpan == config->aimbot[weaponIndex].extrapointdist) {
                        enemy.record->multiExpan = config->aimbot[weaponIndex].extrapointdist - .1;
                    }
                    else {
                        multiplier = enemy.record->multiExpan - (config->aimbot[weaponIndex].extrapointdist * .1);
                        enemy.record->multiExpan = multiplier;
                    }

                    if ((multiplier > config->aimbot[weaponIndex].extrapointdist) || (multiplier < .1)) {
                        multiplier = config->aimbot[weaponIndex].extrapointdist;
                        enemy.record->multiExpan = multiplier;
                    }
                }
            }
            else if ((enemy.entity_ptr->velocity().length2D() > 1.01) || (enemy.record->prevVelocity > 1.01)) {

                //enemy.record->multiExpan = config->aimbot[weaponIndex].extrapointdist;
                //multiplier = config->aimbot[weaponIndex].extrapointdist;


            }
        }
        Vector bonePointsMulti[Multipoints::HITBOX_MAX][Multipoints::MULTIPOINTS_MAX];





        /* So We Aren't Running SetupBones 1 million times*/
        if ((!enemy.bt && !Animations::data.player[enemy.entity_ptr->index()].hasBackup) || config->debug.forcesetupBones) {
            if (!Multipoints::retrieveAll(enemy.entity_ptr, config->aimbot[weaponIndex].multidistance, multiplier, bonePointsMulti, nullptr, false)) {
                continue;
            }
        }
        else if (enemy.bt) {
            if (!Multipoints::retrieveAll(enemy.entity_ptr, config->aimbot[weaponIndex].multidistance, multiplier, bonePointsMulti, backup_rec->matrix, true)) {
                continue;
            }
        }
        else if (Animations::data.player[enemy.entity_ptr->index()].hasBackup) {
            if (!Multipoints::retrieveAll(enemy.entity_ptr, config->aimbot[weaponIndex].multidistance, multiplier, bonePointsMulti, Animations::data.player[enemy.entity_ptr->index()].matrix, true)) {
                continue;
            }
        }
        else {
            if (!Multipoints::retrieveAll(enemy.entity_ptr, config->aimbot[weaponIndex].multidistance, multiplier, bonePointsMulti, nullptr, false)) {
                continue;
            }
        }
        if (config->debug.aimbotcoutdebug) {
            Debug::LogItem item;
            item.PrintToScreen = false;
            item.text.push_back(std::wstring{ L"Made it Past Multipoint set for entity " + std::to_wstring(enemy.entity_ptr->index()) });
            Debug::LOG_OUT.push_front(item);
        }



        // So first off we want to calculate what side we are closest to, I.E. which side of the enemy player model is peaked at us

        /*to do this we first make sure our angle is between 0, 360*/
        /*
        float NewEyeYaw = enemy.entity_ptr->eyeAngles().y;
        if (!(NewEyeYaw > 180)) { // make sure we are in -180,180 range and not 0-360 range already
            NewEyeYaw += 180; // Easy as fuck
        }
        */
        // Since eye yaw is in relation to the world, we can calculate which side faces us pretty easy, ignoring z though
        // I believe the correct way would be to find the angle at which the enemy is the player, then compare that to the
        // view angles 0-360 of the enemy. And then have set defined ranges for certain values, but this works well enough
        /*
               Basically we find the average distance between 2 points, one on x, one on z, to the player. Then whichever had a greater average amount, is farther


               ^^^^^^ Above ended up not getting implemented, would certainly fix the issue of tilted hitboxes though ^^^^^^^^^^

        */

        /*

            To save on processesing time, and not autowalling to  152 points (9 points per box, 18 boxes) on a player we divy them up into sets of 3 points
            To decide which to use, we use the following idea:


                               minmaxmaxmax
                         *---------------------* (max, max)
                         |                     |
                         |                     |
            minminminmax |                     | maxmaxmaxmin
                         |                     |
                         |                     |
                         *---------------------*
                (min, min)     minminmaxmin


            defined in Multipoints.cpp as:

            locations[1] = Vector{min.x, (min.y + max.y) * .5f, ((min.z * max.z) * .5f) };  --> minminminmax
            locations[2] = Vector{(min.x + max.x) * .5f, max.y, ((min.z * max.z) * .5f) };  --> minmaxmaxmax
            locations[3] = Vector{max.x, (min.y + max.y), ((min.z * max.z) * .5f) };        --> maxmaxmaxmin
            locations[4] = Vector{(min.x + max.x) * .5f, min.y, ((min.z * max.z) * .5f) };  --> minminmaxmin

            (min,min) set
            locations[5] = Vector{ min.x, min.y, min.z, };
            locations[6] = Vector{ min.x, min.y, max.z, };
            (max,max) set
            locations[7] = Vector{ max.x, max.y, max.z, };
            locations[8] = Vector{ max.x, max.y, min.z, };
            (max,min) set
            locations[9] = Vector{ max.x, min.y, min.z, };
            locations[10] = Vector{ max.x, min.y, max.z, };
            (min,max) set
            locations[11] = Vector{ min.x, max.y, min.z, };
            locations[12] = Vector{ min.x, max.y, max.z, };

            with additional points declared as having an expansion of 2.0f as:


            locations[13] = Vector{ minexpan.x, minexpan.y,minexpan.z, };
            locations[14] = Vector{ minexpan.x, minexpan.y, maxexpan.z, };

            locations[15] = Vector{ maxexpan.x, maxexpan.y, maxexpan.z, };
            locations[16] = Vector{ maxexpan.x, maxexpan.y, minexpan.z, };

            locations[17] = Vector{ maxexpan.x, minexpan.y, minexpan.z, };
            locations[18] = Vector{ maxexpan.x, minexpan.y, maxexpan.z, };

            locations[19] = Vector{ minexpan.x, maxexpan.y, minexpan.z, };
            locations[20] = Vector{ minexpan.x, maxexpan.y, maxexpan.z, };

            Add +8 to original index to access

                                ------ TODO ------
            In the future I need to write a bypass for this if the enemy is visible.

            By checking what the current roatation is on the enemies axis, and the side nearest to player
            you'll be able to process multiple sets of multipoints if the enemy is at say an angle where i.e. (dist to max, max, and min, min line up. With min,max being nearest)

            Currently checking if max,max and min, min are within a certain range of each other
        */







        int closest = 0;
        if (!enemy.isVisible) {
            float mindist = 99999;
            for (int i = 1; i <= 4; i++) { // Find Point Set with lowest distance
                float distance = localPlayer->origin().distTo(bonePointsMulti[Multipoints::HITBOX_STERNUM][i]);
                if (distance < mindist) {
                    mindist = distance;
                    closest = i;
                }
            }
        }
        if ((closest == 0) && !enemy.isVisible) { // if nothing was ever found???? Continue obviously.
            continue;
        }




        //if (!enemy.onshot && config->aimbot[weaponIndex].onshot && (enemy.entity_ptr->getVelocity().length2D() < 5.0f) && (enemy.health < 60)) // if the enemy isnt shooting & onshot is on, and we dont believe its better to override, continue
        //    continue;

        bool baimTriggered = false; // set up baim 
        record = enemy.record;
        if (record && !record->invalid) {
            if (enemy.bt && config->aimbot[weaponIndex].baim && !enemy.onshot) { // if going for bt, just baim
                baimTriggered = true;
            }
            if (enemy.entity_ptr->velocity().length2D() < 10.0f) {
                if ((record->missedshots > config->aimbot[weaponIndex].baimshots) && (!enemy.onshot)) {
                    baimTriggered = true;
                }
                else if ((enemy.health < config->aimbot[weaponIndex].minDamage) && (!enemy.onshot))
                    baimTriggered = true;
            }
        }


        std::vector<int> Hitboxes;
        std::vector<int> Points = { 0 }; // always check go for center of hitbox first, and the middle spot of the hitbox


        if (baimTriggered) {
            Hitboxes = {
                    Multipoints::HITBOX_PELVIS,
                    Multipoints::HITBOX_KIDNEYS,
                    Multipoints::HITBOX_ABDOMEN,
                    Multipoints::HITBOX_STERNUM,
                    Multipoints::HITBOX_CLAVICLES
            };
        }
        else if (config->aimbot[weaponIndex].bone == 8) { // if we are in hitbox mode
            for (int i = 0; i < HITBOX_MAX; i++) { // setup those hitboxes
                if (config->aimbot[weaponIndex].hitboxes[i] == true) {
                    Hitboxes.push_back(i);
                }
            }
        }
        else if (enemy.isVisible) { // if we can see the bastard, why would we shoot at his fuckin' ankles? 

            Hitboxes = {
                Multipoints::HITBOX_HEAD,
                Multipoints::HITBOX_NECK,
                Multipoints::HITBOX_STERNUM,
                Multipoints::HITBOX_CLAVICLES,
                Multipoints::HITBOX_ABDOMEN,
                Multipoints::HITBOX_KIDNEYS,
                Multipoints::HITBOX_PELVIS

            };

        }
        else if (enemy.onshot) { // gimmie that head
            Hitboxes = {
                    Multipoints::HITBOX_HEAD,
                    Multipoints::HITBOX_NECK,
                    Multipoints::HITBOX_CLAVICLES,
                    Multipoints::HITBOX_STERNUM
            };
        }
        else {
            Hitboxes = {
                        Multipoints::HITBOX_HEAD,
                        Multipoints::HITBOX_NECK,
                        Multipoints::HITBOX_PELVIS,
                        Multipoints::HITBOX_ABDOMEN,
                        Multipoints::HITBOX_KIDNEYS,
                        Multipoints::HITBOX_STERNUM,
                        Multipoints::HITBOX_CLAVICLES,
                        Multipoints::HITBOX_LEFT_THIGH,
                        Multipoints::HITBOX_RIGHT_THIGH,
                        Multipoints::HITBOX_LEFT_SHIN,
                        Multipoints::HITBOX_RIGHT_SHIN,
                        Multipoints::HITBOX_LEFT_ANKLE,
                        Multipoints::HITBOX_RIGHT_ANKLE,
                        Multipoints::HITBOX_LEFT_HAND,
                        Multipoints::HITBOX_RIGHT_HAND,
                        Multipoints::HITBOX_LEFT_ARM,
                        Multipoints::HITBOX_LEFT_FOREARM,
                        Multipoints::HITBOX_RIGHT_ARM,
                        Multipoints::HITBOX_RIGHT_FOREARM
            };
        }

        if (!enemy.wasTargeted) { // if we werent targeting this guy, lets just do these hitboxes
            Hitboxes = {
                Multipoints::HITBOX_HEAD,
                Multipoints::HITBOX_PELVIS,
                Multipoints::HITBOX_ABDOMEN,
                Multipoints::HITBOX_STERNUM,
            };
        }


        if (config->aimbot[weaponIndex].pelvisAimOnLBYUpdate && enemy.lbyUpdated) {
            Hitboxes = {
                    Multipoints::HITBOX_PELVIS,
                    Multipoints::HITBOX_ABDOMEN,
                    Multipoints::HITBOX_STERNUM,
                    Multipoints::HITBOX_HEAD,
            };
        }

        if (Hitboxes.empty()) {
            continue;
        }
        if (config->debug.aimbotcoutdebug) {

            Debug::LogItem item2;
            item2.PrintToScreen = false;
            item2.text.push_back(std::wstring{ L"Made it Past Hitbox set for entity " + std::to_wstring(enemy.entity_ptr->index()) });
            Debug::LOG_OUT.push_front(item2);
        }
        // Here's the fun shit, deciding which point sets to shoot at
        // --- TODO --- Take into account for hitboxes at an angle
        if (!enemy.isVisible && enemy.wasTargeted) { // Look, if we are starin this dude in the eyes. Who cares about scanning the corner of his ear, when we can just shoot him in between the eyes.
                                                   // Do what Dahmer couldn't, and lobotomize him
                                                   // Also fuck all this if he isnt our main target, we will just scan for base points, m'kay?
            switch (closest) {
            case 1:
                Points.push_back(5);
                Points.push_back(6);
                Points.push_back(11);
                Points.push_back(12);
                break;
            case 2:
                Points.push_back(12);
                Points.push_back(11);
                Points.push_back(7);
                Points.push_back(8);
                break;
            case 3:
                Points.push_back(7);
                Points.push_back(8);
                Points.push_back(9);
                Points.push_back(10);
                break;
            case 4:
                Points.push_back(9);
                Points.push_back(10);
                Points.push_back(5);
                Points.push_back(6);
                break;
            default:
                break;
            }
        }
        else {
            Points = { 0 };
        }
        //float minminmaxmaxdeviation = 
        if (config->debug.aimbotcoutdebug) {
            Debug::LogItem item3;
            item3.PrintToScreen = false;
            item3.text.push_back(std::wstring{ L"Made it Past Point set for entity " + std::to_wstring(enemy.entity_ptr->index()) });
            Debug::LOG_OUT.push_front(item3);
        }

        // This code here will sort these points based on the players velocity, and what portion of the body is expected to peak first
        Vector Velocity = enemy.entity_ptr->getVelocity();

        if ((Velocity.length2D() > 30.01f) && (Points.size() > 1)) {
            bool axis = ((abs(Velocity.y) - abs(Velocity.x)) > 0) ? 1 : 0; // True for Y Axis, False for X Axis

            int side = axis ? (Velocity.y > 1) : (Velocity.x > 1); // 1 for Pos, 0 for neg
            side = side ? 1 : -1; // Shit if 0 to -1


            /* Takes the Chest Hitbox*/
            /* If you pass a point that is less than 0 or greater than 21, you'll crash. So dont do that*/
            struct point_sort {
            private:
                bool axis;
                int side;
                std::array<Vector, Multipoints::MULTIPOINTS_MAX>Box;
            public:
                point_sort(bool ax, int si, std::array<Vector, Multipoints::MULTIPOINTS_MAX>Vec) {
                    this->axis = ax;
                    this->side = si;
                    std::array<Vector, Multipoints::MULTIPOINTS_MAX> Box = Vec;
                    this->Box = Box;
                }
                bool operator() (int A, int B) {
                    float a = side ? Box[A].y : Box[A].x; // Get the correct Axis
                    float b = side ? Box[B].y : Box[B].x;
                    if ((A == 0) && (B != 0)) // Sort middle of hitbox to always be first
                        return true;
                    if ((b * side) > (a * side)) // Get best point; 1000 > 50; but -50 > -1000
                        return true;
                    if ((b * side) < (a * side))
                        return false;
                    if ((b * side) == (a * side))
                        if (Box[B].x > Box[A].x) // crappy fix for weak order sorting or whatever its called
                            return true;
                    return false;
                }
            };
            std::array<Vector, Multipoints::MULTIPOINTS_MAX> PointArray;
            std::copy(std::begin(bonePointsMulti[Multipoints::HITBOX_STERNUM]), std::end(bonePointsMulti[Multipoints::HITBOX_STERNUM]), std::begin(PointArray));
            std::sort(Points.begin(), Points.end(), point_sort(axis, side, PointArray));
        }
        if (config->debug.aimbotcoutdebug) {
            Debug::LogItem item4;
            item4.PrintToScreen = false;
            item4.text.push_back(std::wstring{ L"Made it Past Velo Point Sort for entity " + std::to_wstring(enemy.entity_ptr->index()) });
            Debug::LOG_OUT.push_front(item4);
        }


        bestDamage = -1.0f;
        bool killFound = false;
        bool visFound = false;
        bool breakLoop = false;
        int totalPointsScanned = 0;


        for (int l = 0; l <= 1; l++) { /* For Velo Point Sort*/
            std::vector<int>Points_Sp;
            if (Points.size() == 1) {
                Points_Sp = { Points.at(0) };
            }
            else {
                Points_Sp = { Points.at(0 + (l * 2)) , Points.at(1 + (l * 2)) }; // This is a shit solution, need to rewrite
            }

            for (int i = 0; i < Hitboxes.size(); i++) { /*Iterate hitboxes*/

                auto hitbox = Hitboxes[i];

                for (int j = 0; ((j < ((config->aimbot[weaponIndex].multiincrease == true) ? 2 : 1))); j++) { /* For Extended Multipoints */
                    for (auto point : Points_Sp) { /* Iterate Points*/
                        totalPointsScanned++;
                        if ((totalPointsScanned > config->aimbot[weaponIndex].pointstoScan) && (bestDamage < 1)) {
                            breakLoop = true;
                            break;
                        }

                        Vector vPoint = bonePointsMulti[hitbox][point + (((j == 1) || (point == 0)) ? 0 : 8)];
                        float damage = Autowall->Damage(vPoint, wallbangVector, 0.0f);


                        if (config->debug.aimbotcoutdebug) {
                            Debug::LogItem item;
                            item.PrintToScreen = false;
                            item.text.push_back(std::wstring{ L"Ran AutoWall on entity " + std::to_wstring(enemy.entity_ptr->index()) + L" Got damage: " + std::to_wstring(damage) + L" Best Damage: " + std::to_wstring(bestDamage)});
                            Debug::LOG_OUT.push_front(item);
                        }

                        if ((damage > config->aimbot[weaponIndex].minDamage)) {

                            if (config->aimbot[weaponIndex].ensureHC) {
                                const auto aimPunch = activeWeapon->requiresRecoilControl() ? localPlayer->getAimPunch() : Vector{ };
                                auto angle = calculateRelativeAngle(localPlayer->getEyePosition(), vPoint, cmd->viewangles + aimPunch);

                                if (fabs(angle.x) > config->misc.maxAngleDelta || fabs(angle.y) > config->misc.maxAngleDelta) {
                                        angle.x = std::clamp(angle.x, -config->misc.maxAngleDelta, config->misc.maxAngleDelta);
                                        angle.y = std::clamp(angle.y, -config->misc.maxAngleDelta, config->misc.maxAngleDelta);

                                 }
                                if (!hitChance(localPlayer.get(), enemy.entity_ptr, activeWeapon, angle, cmd, config->aimbot[weaponIndex].hitChance) > config->aimbot[weaponIndex].hitChance) {
                                    continue;
                                }
                            }

                            if (damage > bestDamage) {
                                bestDamage = damage;
                                BestPoint = vPoint;
                                Target = enemy.entity_ptr;
                                if ((config->aimbot[weaponIndex].bone == 0) || localPlayer->canSeePoint(vPoint)) {
                                    breakLoop = true;
                                    if (config->debug.aimbotcoutdebug) { Debug::LogItem item3; item3.PrintToScreen = false; item3.text.push_back(std::wstring{ L"[1384] Breaking Loop " + std::to_wstring(enemy.entity_ptr->index()) }); Debug::LOG_OUT.push_front(item3); }
                                }
                                break;
                            }



                        } else if (damage > (enemy.health + 5)) {
                             const auto aimPunch = activeWeapon->requiresRecoilControl() ? localPlayer->getAimPunch() : Vector{ };
                             auto angle = calculateRelativeAngle(localPlayer->getEyePosition(), vPoint, cmd->viewangles + aimPunch);

                             if (fabs(angle.x) > config->misc.maxAngleDelta || fabs(angle.y) > config->misc.maxAngleDelta) {
                                    angle.x = std::clamp(angle.x, -config->misc.maxAngleDelta, config->misc.maxAngleDelta);
                                    angle.y = std::clamp(angle.y, -config->misc.maxAngleDelta, config->misc.maxAngleDelta);

                             }
                             if (hitChance(localPlayer.get(), enemy.entity_ptr, activeWeapon, angle, cmd, config->aimbot[weaponIndex].hitChance) > 80.f) {
                                 if (damage > bestDamage) {
                                     bestDamage = damage;
                                     BestPoint = vPoint;
                                     Target = enemy.entity_ptr;
                                     if ((config->aimbot[weaponIndex].bone == 0) || localPlayer->canSeePoint(vPoint)) {
                                         if (config->debug.aimbotcoutdebug) { Debug::LogItem item3; item3.PrintToScreen = false; item3.text.push_back(std::wstring{ L"[1406] Breaking Loop " + std::to_wstring(enemy.entity_ptr->index()) }); Debug::LOG_OUT.push_front(item3); }
                                         breakLoop = true;
                                     }
                                     break;
                                 }
                             }

                         }

                        if ((bestDamage > 0) && (hitbox != 0)) {
                            break;
                        }
                        if (breakLoop) { break; } /* Is this the one use-case of the goto? Guess I still shouldn't use it, huh? Maybe Use a lambda in the future?*/
                    }
                    if (breakLoop) { break; }
                }
                if ((i > 13) && (config->aimbot[weaponIndex].optimize)) {
                    if (config->debug.aimbotcoutdebug) { Debug::LogItem item3; item3.PrintToScreen = false; item3.text.push_back(std::wstring{ L"[1423] Breaking Loop " + std::to_wstring(enemy.entity_ptr->index()) }); Debug::LOG_OUT.push_front(item3); }
                    breakLoop = true;
                    break;
                }

                if (breakLoop) {break;}
                if ((hitbox != 0) &&
                    (hitbox != 1) &&
                    (hitbox != 2) &&
                    (hitbox != 3) &&
                    (hitbox != 4) &&
                    (hitbox != 5) &&
                    (hitbox != 12) &&
                    (hitbox != 13) &&
                    (hitbox != 16) &&
                    (hitbox != 18) && config->aimbot[weaponIndex].optimize) {
                    breakLoop = true;
                    break;
                }
            }
            if (breakLoop) { break; }
            if (Points.size() == 1) {
                if (config->debug.aimbotcoutdebug) { Debug::LogItem item3; item3.PrintToScreen = false; item3.text.push_back(std::wstring{ L"[1445] Breaking Loop " + std::to_wstring(enemy.entity_ptr->index()) }); Debug::LOG_OUT.push_front(item3); }
                break;
            }

        }

        Save = enemy;
        if (Target != localPlayer.get()) {
            break;
        }




    }

    for (int i = 0; i < Resolver::PlayerRecords.size(); i++) {
            Resolver::Record* record_b = &Resolver::PlayerRecords.at(i);
            if (record_b && !record_b->invalid) {
                record_b->wasTargeted = false;
                record_b->FiredUpon = false;
            }
    }

   if (Target != nullptr) {
            if (Target == localPlayer.get()) {
                if (!enemies.empty()) {
                    enemies.front().record->wasTargeted = true;
                }
                if ((enemies.size() > 1) && (bestDamage < 5.0)) {
                    if (enemies.at(1).distance < enemies.front().distance) {
                        enemies.front().record->wasTargeted = true;
                        enemies.at(1).record->wasTargeted = true;
                        enemies.at(1).record->FiredUpon = false;
                        enemies.front().record->FiredUpon = false;
                        Resolver::TargetedEntity = enemies.at(1).entity_ptr;
                    }
                }
                else if (enemies.size() == 1) {
                    enemies.front().record->wasTargeted = true;
                    enemies.front().record->FiredUpon = false;
                    Resolver::TargetedEntity = enemies.front().entity_ptr;
                }

                for (auto enemy : enemies) {
                    if (enemy.bt == true) {
                        Animations::finishSetup(enemy.entity_ptr);
                    }
                }

                return;
            }

            if (config->debug.aimbotcoutdebug) {
                Debug::LogItem item3;
                item3.PrintToScreen = false;
                std::wstring name = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(Target->getPlayerName(true));
                item3.text.push_back(std::wstring{ L"Selected " + name + L"As Target" });
                Debug::LOG_OUT.push_front(item3);
            }

            if (!(record->invalid) && config->debug.animstatedebug.resolver.enabled) {
                record->wasTargeted = true;
                Resolver::TargetedEntity = Target;
            }
            //Vector Angle = Math::CalcAngle(localPlayer->getEyePosition(), BestPoint);
            static float MinimumVelocity = 0.0f;
            MinimumVelocity = localPlayer->getActiveWeapon()->getWeaponData()->maxSpeedAlt * 0.34f;
            const auto aimPunch = activeWeapon->requiresRecoilControl() ? localPlayer->getAimPunch() : Vector{ };
            auto angle = calculateRelativeAngle(localPlayer->getEyePosition(), BestPoint, cmd->viewangles + aimPunch);
            //angle -= (localPlayer->aimPunchAngle() * interfaces->cvar->findVar("weapon_recoil_scale")->getFloat());
            //angle /= config->aimbot[weaponIndex].smooth;

            bool clamped{ false };

            if (fabs(angle.x) > config->misc.maxAngleDelta || fabs(angle.y) > config->misc.maxAngleDelta) {
                angle.x = std::clamp(angle.x, -config->misc.maxAngleDelta, config->misc.maxAngleDelta);
                angle.y = std::clamp(angle.y, -config->misc.maxAngleDelta, config->misc.maxAngleDelta);
                clamped = true;
            }

            if (localPlayer->velocity().length() >= MinimumVelocity && config->aimbot[weaponIndex].autoStop && (localPlayer->flags() & PlayerFlags::ONGROUND) && localPlayer->isScoped()) {
                Autostop(cmd); //Auto Stop
            }
            if ((activeWeapon->nextPrimaryAttack() <= memory->globalVars->serverTime()) && activeWeapon->isSniperRifle() && !localPlayer->isScoped())
            {
                cmd->buttons |= UserCmd::IN_ATTACK2; //Auto Scope
            }
            //Angle -= (localPlayer->aimPunchAngle() * interfaces->cvar->findVar("weapon_recoil_scale")->getFloat());
            float hitchance = 0.0f;

            if (config->aimbot[weaponIndex].hitChance != 0) {
                hitchance = hitChance(localPlayer.get(), Target, activeWeapon, angle, cmd, config->aimbot[weaponIndex].hitChance);
            }
            else {
                hitchance = config->aimbot[weaponIndex].maxAimInaccuracy >= activeWeapon->getInaccuracy() * 100;
            }

            if ((((hitchance > 0.0f) || config->aimbot[weaponIndex].ensureHC)) && (activeWeapon->nextPrimaryAttack() <= memory->globalVars->serverTime()))
            {
                cmd->viewangles += angle; //Set Angles

                if (!config->aimbot[weaponIndex].silent) {
                    interfaces->engine->setViewAngles(cmd->viewangles);
                }


                if (config->aimbot[weaponIndex].autoShot) {
                    cmd->buttons |= UserCmd::IN_ATTACK; //shoot

                    if (!record->invalid) {
                        record->FiredUpon = true;
                        record->wasTargeted = true;
                    }
                }


                if (cmd->buttons & UserCmd::IN_ATTACK) {
                    Resolver::shotdata shotdata;
                    shotdata.simtime = Target->simulationTime();
                    shotdata.EyePosition = localPlayer->getEyePosition();
                    shotdata.viewangles = cmd->viewangles;
                    shotdata.TargetedPosition = BestPoint;

                    if (Save.bt && backup_rec && !Backtrack::valid(backup_rec->simulationTime)) {
                        memcpy(shotdata.matrix, backup_rec->matrix, sizeof(matrix3x4) * std::clamp(backup_rec->countBones, 0, 256));
                        record->shots.push_front(shotdata);
                    }
                    else if (Animations::data.player[Target->index()].hasBackup) {
                        memcpy(shotdata.matrix, Animations::data.player[Target->index()].matrix, sizeof(matrix3x4) * std::clamp(Animations::data.player[Target->index()].countBones, 0, 256));
                        record->shots.push_front(shotdata);
                    }
                    else {
                        Target->setupBones(shotdata.matrix, 256, 0x7FF00, memory->globalVars->currenttime); /* TODO: Don't Call Setup Bones Here, back it up somewhere*/
                        record->shots.push_front(shotdata);
                    }

                }
            }
            else {
                if (!enemies.empty()) {
                    //enemies.front().record->wasTargeted = true;
                    //Resolver::TargetedEntity = enemies.front().entity_ptr;
                    record->wasTargeted = true;
                }
            }

            for (auto enemy : enemies) {
                if (enemy.bt == true) {
                    Animations::finishSetup(enemy.entity_ptr);
                }
            }


            //if ((Target->velocity().length2D() > 170) && (!config->backtrack.enabled) && config->debug.movefix)
            //   Animations::finishSetup(Target);

        }


    
}
    /*
        if ((Target != interfaces->entityList->getEntity(localPlayer->index())) && Target->isAlive() && (activeWeapon->nextPrimaryAttack() <= memory->globalVars->serverTime())) //if has taget
        {
            int bestDmg = 0;
            int bestChance = 0;
            Vector bestAngle = { 0, 0, 0 };
            Vector bestModifiedAngle{ 0, 0, 0 };

            float MinimumVelocity = localPlayer->getActiveWeapon()->getWeaponData()->maxSpeedAlt * 0.34f;

            if ((localPlayer->getVelocity().length() >= MinimumVelocity) && config->aimbot[weaponIndex].autoStop && (localPlayer->flags() & PlayerFlags::ONGROUND))
            {
                Autostop(cmd);
            }

            if (activeWeapon->isSniperRifle() && !localPlayer->isScoped())
            {
                cmd->buttons |= UserCmd::IN_ATTACK2;
            }
            VectorAndDamage Point;
            Point.vec = BestPoint;
            Point.damage = bestDamage;
            AimPoints = { Point };
            for (auto const& aimPoint : AimPoints) {
                int raysHit = 0;
                Vector Angle = Math::CalcAngle(localPlayer->getEyePosition(), aimPoint.vec);

                //No recoil
                Angle -= (localPlayer->aimPunchAngle() * interfaces->cvar->findVar("weapon_recoil_scale")->getFloat());
                Vector bestVector{ Angle.x, Angle.y, Angle.z };

                if (HitChance(Angle, Target, activeWeapon, weaponIndex, cmd, config->aimbot[weaponIndex].hitChance, bestHitchance, firstWallToPen, raysHit, bestVector)) // || HitChance(Angle, Target, activeWeapon, weaponIndex, cmd, config->ragebot[weaponIndex].hitChance, bestHitchance, firstWallToPen, raysHit, bestVector)
                {
                    if (aimPoint.damage > bestDmg)
                    {
                        bestDmg = aimPoint.damage;
                        bestChance = bestHitchance;
                        bestAngle = Angle;
                        bestModifiedAngle = bestVector;
                    }
                }
            }


            if ((bestDmg > 0) && (config->aimbot[weaponIndex].autoShot || cmd->buttons & UserCmd::IN_ATTACK))
            {
                cmd->viewangles = bestAngle;//bestModifiedAngle;

                if (!config->aimbot[weaponIndex].silent)
                {
                    interfaces->engine->setViewAngles(cmd->viewangles);
                }

                cmd->buttons |= UserCmd::IN_ATTACK; //shoot

                if (config->debug.animstatedebug.resolver.enabled && !(record->invalid)) {
                    record->FiredUpon = true;
                }
            }
            */
        



        /*
        if (Target != nullptr) {
            if (Target == localPlayer.get())
                return;

            Vector Angle = Math::CalcAngle(localPlayer->getEyePosition(), BestPoint);
            static float MinimumVelocity = 0.0f;
            MinimumVelocity = localPlayer->getActiveWeapon()->getWeaponData()->maxSpeedAlt * 0.34f;



            if (localPlayer->velocity().length() >= MinimumVelocity && config->aimbot[weaponIndex].autoStop && (localPlayer->flags() & PlayerFlags::ONGROUND))
                Autostop(cmd); //Auto Stop

            if (activeWeapon->nextPrimaryAttack() <= memory->globalVars->serverTime() && activeWeapon->isSniperRifle() && !localPlayer->isScoped())
            {
                cmd->buttons |= UserCmd::IN_ATTACK2; //Auto Scope
            }

            Angle -= (localPlayer->aimPunchAngle() * interfaces->cvar->findVar("weapon_recoil_scale")->getFloat());
            bool hitchance = true;

            if (config->aimbot[weaponIndex].hitChance != 0.0f) {
                hitchance = HitChance(Angle, Target, activeWeapon, weaponIndex, cmd, (const int)config->aimbot[weaponIndex].hitChance);
            }
            else {
                hitchance = (config->aimbot[weaponIndex].maxAimInaccuracy >= activeWeapon->getInaccuracy());
            }

            if ( hitchance && activeWeapon->nextPrimaryAttack() <= memory->globalVars->serverTime())
            {
                cmd->viewangles = Angle; //Set Angles

                if (!config->aimbot[weaponIndex].silent)
                    interfaces->engine->setViewAngles(cmd->viewangles);

                cmd->buttons |= UserCmd::IN_ATTACK; //shoot

                if (config->debug.animstatedebug.resolver.enabled && !(record->invalid)) {
                    record->FiredUpon = true;
                }
            }




        }
        */





















/*
                //std::ofstream myfile;
                //myfile.open("C:\\Users\\userWIN\\source\\repos\\test\\Debug\\aimbotlog.txt", std::ios_base::app);

                // myfile << "Get Model Function called\n";

const auto model = entity->getModel();
if (!model) {
    //myfile << "Get Model returned null model, exiting\n";
    return;
}
//myfile << "Calling getStudioModel\n";
const auto studioModel = interfaces->modelInfo->getStudioModel(model);
if (!studioModel) {
    //myfile << "getStudioModel retyrbed null model\n";
    return;
}

// StudioHitboxSet* getHitboxSet(int i)
// myfile << "Calling getHitboxSet\n";
const auto studiohitboxset = studioModel->getHitboxSet(0);
if (!studiohitboxset) {
    // myfile << "getHitboxSet returned null \n";
    return;
}

//if (!(matrix3x4 boneMatrices[256]; entity->setupBones(boneMatrices, 256, 256, 0.0f)))
//   return;

// Multipoint StudioHitBoxCode 
float maxDamage = 0;
float damage = 0;

for (auto bone : boneList) {

    //myfile << "bone = " << bone << "\n";
    auto studiobone = studiohitboxset->getHitbox(config->aimbot[weaponIndex].bone > 1 ? 10 - config->aimbot[weaponIndex].bone : bone);

    if (!studiobone) {
        //myfile << "No Studiobone returned from getHitBox\n";
        return;
    }

    //myfile << "studiobone->bone gives " << studiobone->bone;


    matrix3x4 boneMatrices[256];
    entity->setupBones(boneMatrices, 256, 256, 0.0f);

    auto basebonePosition = entity->getBonePosition(studiobone->bone);

    //myfile << "\nbasebonePosition = " << basebonePosition.x << " " << basebonePosition.y << " " << basebonePosition.z;



   // myfile << "\nstudio->bbMin " << studiobone->bbMin.x << " " << studiobone->bbMin.y <<  " " << studiobone->bbMin.z;
    //myfile << "\nstudio->bbMax " << studiobone->bbMax.x << " " << studiobone->bbMax.y << " " << studiobone->bbMax.z;

    Vector bbmin = studiobone->bbMin * config->aimbot[weaponIndex].multidistance;
    Vector bbmax = studiobone->bbMax * config->aimbot[weaponIndex].multidistance;

    Vector points[] = { Vector{bbmin.x, bbmin.y, bbmin.z},
         Vector{bbmin.x, bbmax.y, bbmin.z},
         Vector{bbmax.x, bbmax.y, bbmin.z},
         Vector{bbmax.x, bbmin.y, bbmin.z},
         Vector{bbmax.x, bbmax.y, bbmax.z},
         Vector{bbmin.x, bbmax.y, bbmax.z},
         Vector{bbmin.x, bbmin.y, bbmax.z},
         Vector{bbmax.x, bbmin.y, bbmax.z},
    };

    //auto bonePosition = entity->getBonePosition(config->aimbot[weaponIndex].bone > 1 ? 10 - config->aimbot[weaponIndex].bone : bone);
    Vector bonePositions[8];
    bonePositions[0] = basebonePosition;
    int i = 1;
    for (auto point : points) {
        bonePositions[i] = point.transform(boneMatrices[bone]);
        //myfile << "bonePosition: " << i << " is " << bonePositions[i].x << " " << bonePositions[i].y << " " << bonePositions->z << "\n";
        i++;
    }

    //Vector bonePositions[] = { (basebonePosition - (studiobone->bbMin * config->aimbot[weaponIndex].multidistance)), basebonePosition, (basebonePosition + (studiobone->bbMax * config->aimbot[weaponIndex].multidistance)) };





    for (auto bonePosition : bonePositions) {

        // myfile << "\nCurrent Bone Position in multipoint is: " << bonePosition.x << " " << bonePosition.y << " " << bonePosition.z << "\n";
        if (!entity->isVisible(bonePosition) && (config->aimbot[weaponIndex].visibleOnly || !canScan(entity, bonePosition, activeWeapon->getWeaponData(), config->aimbot[weaponIndex].killshot ? entity->health() : config->aimbot[weaponIndex].minDamage, config->aimbot[weaponIndex].friendlyFire, damage)))
            continue;

        auto angle = calculateRelativeAngle(localPlayerEyePosition, bonePosition, cmd->viewangles + aimPunch);
        auto fov = std::hypotf(angle.x, angle.y);
        if (fov < bestFov) {
            bestFov = fov;
            if ((damage > maxDamage)) {
                bestTarget = bonePosition;
                maxDamage = damage;

            }

            if ((damage > 100.0f) || (damage > entity->health())) {
                bestTarget = bonePosition;
                maxDamage = damage;
                break;
            }

        }
        // myfile << "Curr Damage is: " << damage << " best damage is: " << maxDamage << "\n";


    }

    //myfile << "Best target is " << bestTarget.x << " " << bestTarget.y << " " << bestTarget.z << "\n";
   // myfile.close();
    if (config->aimbot[weaponIndex].bone || (damage > 100.0f) || (damage > entity->health()))
        break;

    */

        /*
    for (float i = config->aimbot[weaponIndex].multidistance; i < 3; i += config->aimbot[weaponIndex].multidistance) {


        if (!entity->isVisible(bonePosition) && (config->aimbot[weaponIndex].visibleOnly || !canScan(entity, bonePosition, activeWeapon->getWeaponData(), config->aimbot[weaponIndex].killshot ? entity->health() : config->aimbot[weaponIndex].minDamage, config->aimbot[weaponIndex].friendlyFire, damage)))
            continue;

        auto angle = calculateRelativeAngle(localPlayerEyePosition, bonePosition, cmd->viewangles + aimPunch);
        auto fov = std::hypotf(angle.x, angle.y);
        if (fov < bestFov) {
            bestFov = fov;
            if (damage > maxDamage) {
                bestTarget = bonePosition;
                maxDamage = damage;
            }
        }
        if (config->aimbot[weaponIndex].bone)
            break;
        Vector adder;
        adder.x = i;
        adder.y = 0.0f;
        adder.z = 0.0f;
        bonePosition += adder;
    }
    */