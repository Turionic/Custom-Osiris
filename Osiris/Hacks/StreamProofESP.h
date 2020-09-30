#pragma once
#include "../SDK/GameEvent.h"
#include "../SDK/Vector.h"
#include "../Config.h"
#include "../ConfigStructs.h"
#include "../SDK/Entity.h"
#include "../SDK/matrix3x4.h"
#include <vector>
namespace StreamProofESP
{
    void render() noexcept;
    void bulletTracer(GameEvent* event) noexcept;

    
    void DrawESPItemList() noexcept;

    /*
    
    
    */

    enum DrawType {
        LINE,
        BOX,
        CIRCLE,
        SPHERE,
        TRIANGLE,
        PYRAMID,
        POLYGON,
        HITBOX,
        SKELETON
    };

    enum Flags {
        EXPANDABLE = 0b00000001,
    };

    struct DrawItem {
        int invalid = false;
        int type;
        int BoundType = 0; // is Bound to entity? 0 = no, 1 = yes, 2 = 2 entities
        int segements;
        int flags;
        bool Fade = false;
        float thickness = 1.0f;
        float creation_time = 0.0f;
        float exist_time = 0.0f;
        float radius = 1.0f;
        float height = 1.0f;
        float width = 1.0f;
        matrix3x4 matrix[256];
        std::vector<std::pair<Vector, Vector>> bones;
        Entity* boundstartEntity = 0;
        Entity* boundendEntity = 0;
        Vector StartPos = { 0,0,0 };
        Vector EndPos = { 0,0,0 };
        ColorA Color;
    };

    void SetupCurrentMatrix(DrawItem& Object, Entity* entity);

    extern std::vector<struct DrawItem> ESPItemList;
    extern ImVec2 ScreenSize;
}
