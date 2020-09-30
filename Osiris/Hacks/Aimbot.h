#pragma once
#include <vector>
#include "StreamProofESP.h"
struct UserCmd;
struct Vector;
class Entity;

namespace Aimbot
{
    Vector calculateRelativeAngle(const Vector& source, const Vector& destination, const Vector& viewAngles) noexcept;
    bool GetAllPoints(Entity* entity, int weaponIndex, std::vector<Vector>& AimPoint);
    bool GetBestPoint(Entity* entity, int weaponIndex, std::vector<Vector>& AimPoints, Vector &BestPoint);
    float willPointWork(Entity* entity, int weaponIndex, Vector AimPoint);
    bool ScanAllPoints(Entity* entity, int weaponIndex, Vector &AimPoint, int bone, bool wasTargeted);
    void run(UserCmd*, int& bestDamage, int& bestHitchance, Vector& wallbangVector) noexcept;
    void oldstyle(UserCmd* cmd) noexcept;
    bool BaseLogic(int entity_indx, int weaponIndex, std::vector<Vector> AimPoint, Vector& BestPoint, int bone, bool wasTargeted);
    //static bool HitChance(Vector angles, Entity* entity, Entity* weapon, int weaponIndex, UserCmd* cmd, const int chance) noexcept;
    static float hitChance(Entity* localPlayer, Entity* entity, Entity* weaponData, const Vector& destination, const UserCmd* cmd, const int hitChance) noexcept;
    void Autostop(UserCmd* cmd) noexcept;

    extern ImVec2 ScreenSize;
}

enum bones : int {
    pelvis = 0,
    spine_0 = 3,
    spine_1 = 4,
    spine_2 = 5,
    spine_3 = 6,
    neck = 7,
    head = 8,
    clavicle_l = 9,
    arm_upper_l = 10,
    arm_lower_l = 11,
    hand_l = 12,
    finger_middle_meta_l = 13,
    finger_middle_0_l = 14,
    finger_middle_1_l = 15,
    finger_middle_2_l = 16,
    finger_pinky_meta_l = 17,
    finger_pinky_0_l = 18,
    finger_pinky_1_l = 19,
    finger_pinky_2_l = 20,
    finger_index_meta_l = 21,
    finger_index_0_l = 22,
    finger_index_1_l = 23,
    finger_index_2_l = 24,
    finger_thumb_0_l = 25,
    finger_thumb_1_l = 26,
    finger_thumb_2_l = 27,
    finger_ring_meta_l = 28,
    finger_ring_0_l = 29,
    finger_ring_1_l = 30,
    finger_ring_2_l = 31,
    weapon_hand_l = 32,
    arm_lower_l_twist = 33,
    arm_lower_l_twist1 = 34,
    arm_upper_l_twist = 35,
    arm_upper_l_twist1 = 36,
    clavicle_r = 37,
    arm_upper_r = 38,
    arm_lower_r = 39,
    hand_r = 40,
    finger_middle_meta_r = 41,
    finger_middle_0_r = 42,
    finger_middle_1_r = 43,
    finger_middle_2_r = 44,
    finger_pinky_meta_r = 45,
    finger_pinky_0_r = 46,
    finger_pinky_1_r = 47,
    finger_pinky_2_r = 48,
    finger_index_meta_r = 49,
    finger_index_0_r = 50,
    finger_index_1_r = 51,
    finger_index_2_r = 52,
    finger_thumb_0_r = 53,
    finger_thumb_1_r = 54,
    finger_thumb_2_r = 55,
    finger_ring_meta_r = 56,
    finger_ring_0_r = 57,
    finger_ring_1_r = 58,
    finger_ring_2_r = 59,
    weapon_hand_r = 60,
    arm_lower_r_twist = 61,
    arm_lower_r_twist1 = 62,
    arm_upper_r_twist = 63,
    arm_upper_r_twist1 = 64,
    leg_upper_l = 65,
    leg_lower_l = 66,
    ankle_l = 67,
    ball_l = 68,
    lfoot_lock = 69,
    leg_upper_l_twist = 70,
    leg_upper_l_twist1 = 71,
    leg_upper_r = 72,
    leg_lower_r = 73,
    ankle_r = 74,
    ball_r = 75,
    rfoot_lock = 76,
    leg_upper_r_twist = 77,
    leg_upper_r_twist1 = 78,
    finger_pinky_l_end = 79,
    finger_thumb_l_end = 80,
    finger_pinky_r_end = 81,
    finger_thumb_r_end = 82,
    ValveBiped_weapon_bone = 83,
    lh_ik_driver = 84,
    primary_jiggle_jnt = 85
};

enum Hitbox : int
{
    HITBOX_HEAD,
    HITBOX_NECK,
    HITBOX_LOWER_NECK,
    HITBOX_PELVIS,
    HITBOX_BODY,
    HITBOX_THORAX,
    HITBOX_CHEST,
    HITBOX_UPPER_CHEST,
    HITBOX_RIGHT_THIGH,
    HITBOX_LEFT_THIGH,
    HITBOX_RIGHT_CALF,
    HITBOX_LEFT_CALF,
    HITBOX_RIGHT_FOOT,
    HITBOX_LEFT_FOOT,
    HITBOX_RIGHT_HAND,
    HITBOX_LEFT_HAND,
    HITBOX_RIGHT_UPPER_ARM,
    HITBOX_RIGHT_FOREARM,
    HITBOX_LEFT_UPPER_ARM,
    HITBOX_LEFT_FOREARM,
    HITBOX_MAX
};