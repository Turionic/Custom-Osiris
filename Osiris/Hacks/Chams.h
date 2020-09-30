#pragma once

#include <vector>

#include "../Config.h"

class Entity;
struct ModelRenderInfo;
class matrix3x4;
class Material;

class Chams {
public:
    Chams() noexcept;
    bool render(void*, void*, const ModelRenderInfo&, matrix3x4*) noexcept;
private:
    void renderPlayer(Entity* player, bool dead = false) noexcept;
    void renderWeapons() noexcept;
    void renderHands() noexcept;
    void renderSleeves() noexcept;
    void renderSmoke() noexcept;

    enum ChamsId {
        ALLIES = 0,
        ENEMIES,
        PLANTING,
        DEFUSING,
        LOCALPLAYER,
        WEAPONS,
        HANDS,
        BACKTRACK,
        SLEEVES
    };

    Material* normal;
    Material* flat;
    Material* animated;
    Material* platinum;
    Material* glass;
    Material* crystal;
    Material* chrome;
    Material* silver;
    Material* gold;
    Material* plastic;
    Material* glow;
    Material* pearlescent;
    Material* metallic;

    Material* metalsnow;
    Material* glasswindow;
    Material* c4_gift;
    Material* urban_puddle;
    Material* crystal_cube;
    Material* ghost1;

    Material* zombie;
    Material* searchlight;
    Material* brokenglass;

    Material* crystal_blue;
    Material* velvet;
    Material* water;
    Material* metalwall;

    Material* whiteboard01;
    Material* whiteboard04;

    constexpr auto dispatchMaterial(int id) const noexcept
    {
        switch (id) {
        default:
        case 0: return normal;
        case 1: return flat;
        case 2: return animated;
        case 3: return platinum;
        case 4: return glass;
        case 5: return chrome;
        case 6: return crystal;
        case 7: return silver;
        case 8: return gold;
        case 9: return plastic;
        case 10: return glow;
        case 11: return pearlescent;
        case 12: return metallic;
        case 13: return metalsnow;
        case 14: return glasswindow;
        case 15: return c4_gift;
        case 16: return urban_puddle;
        case 17: return crystal_cube;
        case 18: return ghost1;
        case 19: return zombie;
        case 20: return searchlight;
        case 21: return brokenglass;
        case 22: return crystal_blue;
        case 23: return velvet;
        case 24: return water;
        case 25: return metalwall;
        case 26: return whiteboard01;
        case 27: return whiteboard04;
        }
    }

    bool appliedChams;
    void* ctx;
    void* state;
    const ModelRenderInfo* info;
    matrix3x4* customBoneToWorld;

    void applyChams(const std::array<Config::Chams::Material, 7>& chams, int health = 0, matrix3x4* customMatrix = nullptr) noexcept;
};
