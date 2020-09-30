#define NOMINMAX
#include "StreamProofESP.h"

#include "../imgui/imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../imgui/imgui_internal.h"

#include "../Config.h"
#include "../fnv.h"
#include "../GameData.h"
#include "../Helpers.h"
#include "../SDK/Engine.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/Entity.h"
#include "../Memory.h"
#include "Aimbot.h"

#include <limits>
#include <tuple>
#include <math.h>

std::vector<struct StreamProofESP::DrawItem> StreamProofESP::ESPItemList;
static ImDrawList* drawList;


ImVec2 StreamProofESP::ScreenSize = { 0,0 };

static Vector calculateRelativeAngleN(const Vector& source, const Vector& destination, const Vector& viewAngles) noexcept
{
    Vector delta = destination - source;
    Vector angles{ radiansToDegrees(atan2f(-delta.z, std::hypotf(delta.x, delta.y))) - viewAngles.x,
                   radiansToDegrees(atan2f(delta.y, delta.x)) - viewAngles.y };
    angles.normalize();
    return angles;
}
/* Thanks UC! Math Is Way too Hard */
/* NVM This doesnt do what i want*/
void FindPoint(Vector point, ImVec2& Vec)
{
    const auto& screenSize = ImGui::GetIO().DisplaySize;

    int degrees = screenSize.y;

    float x2 = screenSize.x /2;
    float y2 = screenSize.y /2;

    float d = std::sqrt(std::pow((point.x - x2), 2) + (std::pow((point.y - y2), 2))); //Distance
    float r = degrees; //Segment ratio

    point.x = r * point.x + (1 - r) * x2; //find point that divides the segment
    point.y = r * point.y + (1 - r) * y2; //into the ratio (1-r):r

    Vec.x = -point.x;
    Vec.y = point.y;
}

static void FindNewPoint(const Vector& in, ImVec2& out) {
    auto angle = calculateRelativeAngleN(localPlayer->eyeAngles(), in, localPlayer->eyeAngles());

    FindPoint(in, out);
    out.x -= angle.x;
    out.y += angle.z;
}


bool isValid(ImVec2 screen_point) {
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;

    if (screen_point.x > screenSize.x) {
        return false;
    }
    else if (screen_point.x < 0) {
        return false;
    }
    else if (screen_point.y > screenSize.y) {
        return false;
    }
    else if (screen_point.y < 0) {
        return false;
    }

    return true;


}

static ImVec2 clamp_screen_pos(ImVec2 screen_point, ImVec2 screen_reference = ImGui::GetIO().DisplaySize) {
    screen_reference = ImGui::GetIO().DisplaySize;
    if (isValid(screen_point)) return screen_point; //Greater than or Equal to 0 and Less than or Equal to the Screen Height/Width

    bool negativeX = (screen_reference.x < screen_point.x);
    bool negativeY = (screen_reference.y < screen_point.y);

    ImVec2 slope;
    slope.x = negativeX ? -(screen_point.x / screen_reference.x) : (screen_reference.x / screen_point.x);
    slope.y = negativeY ? -(screen_point.y / screen_reference.y) : (screen_reference.y / screen_point.y); //Basic Algebra (Account for an Inverse Slope)

    int i = 0;
    while (!(isValid(screen_point))) { //This will Fail if the End Point Failed the Same Way (Also < 0/Also > Height/Width)
        screen_point += slope; //Keep Moving the Starting Point along the Line Until it is Valid
        i++;
        if (i > 200) {
            return screen_point;
        }
    }
    return screen_point;
}



static bool worldToScreen(const Vector& in, ImVec2& out) noexcept
{
    const auto& matrix = GameData::toScreenMatrix();

    const auto w = matrix._41 * in.x + matrix._42 * in.y + matrix._43 * in.z + matrix._44;



    out = ImGui::GetIO().DisplaySize / 2.0f;
    out.x *= 1.0f + (matrix._11 * in.x + matrix._12 * in.y + matrix._13 * in.z + matrix._14) / w;
    out.y *= 1.0f - (matrix._21 * in.x + matrix._22 * in.y + matrix._23 * in.z + matrix._24) / w;

    if (w < 0.001f) {
        out = clamp_screen_pos(out);
        return false;
    }

    return true;
}

/*
static bool worldToScreenClip(Vector in, Vector in2, ImVec2& out, ImVec2& out2) {
    Vector VecToClip;
    Vector VectoKeep;
    ImVec2* VecToSet;
    ImVec2 VecToNotSet;

    int indicator = 0; // GHETTO AF. 1 = in1, 2 = in2, 3 = both.

    if (!worldToScreen(in, out)) {
        indicator++;
    }
    if (!worldToScreen(in2, out2)) {
        indicator += 2;
    }


    switch (indicator) {
        case 0:
            return true;
        case 1:
            VecToClip = in;
            VecToSet = &out;
            VectoKeep = in2;
            VecToNotSet = out2;
            break;
        case 2:
            VecToClip = in2;
            VecToSet = &out2;
            VectoKeep = in;
            VecToNotSet = out;
            break;
        case 3:
        default:
            return false;
    }
    
    const Matrix4x4& matrix = GameData::toScreenMatrix();
    const auto& screenSize = ImGui::GetIO().DisplaySize;

    ImVec4 Clip;
    Clip.x = VecToClip.x * matrix._11 + VecToClip.y * matrix._12 + VecToClip.z * matrix._13 + matrix._14;
    Clip.y = VecToClip.x * matrix._21 + VecToClip.y * matrix._22 + VecToClip.z * matrix._23 + matrix._24;
    Clip.z = VecToClip.x * matrix._31 + VecToClip.y * matrix._32 + VecToClip.z * matrix._33 + matrix._34;
    Clip.w = VecToClip.x * matrix._41 + VecToClip.y * matrix._42 + VecToClip.z * matrix._43 + matrix._44;

    ImVec4 NonClip;

    NonClip.x = VectoKeep.x * matrix._11 + VectoKeep.y * matrix._12 + VectoKeep.z * matrix._13 + matrix._14;
    NonClip.y = VectoKeep.x * matrix._21 + VectoKeep.y * matrix._22 + VectoKeep.z * matrix._23 + matrix._24;
    NonClip.z = VectoKeep.x * matrix._31 + VectoKeep.y * matrix._32 + VectoKeep.z * matrix._33 + matrix._34;
    NonClip.w = VectoKeep.x * matrix._41 + VectoKeep.y * matrix._42 + VectoKeep.z * matrix._43 + matrix._44;




    float _near = matrix._43 / (matrix._33 - 1.0f);
    //float fov = matrix._11 * (screenSize.x / screenSize.y);
    float fov = matrix._22;

    float n = (Clip.w - _near) / (Clip.w - NonClip.w);

    ImVec4 AfterClip;

    AfterClip.x = (n * Clip.x) + ((1 - n) * NonClip.x);
    AfterClip.y = (n * Clip.y) + ((1 - n) * NonClip.y);
    AfterClip.z = (n * Clip.z) + ((1 - n) * NonClip.z);
    AfterClip.w = _near;

    if (AfterClip.w < 0)
        return false;

    *VecToSet = ImGui::GetIO().DisplaySize / 2.0f;
    VecToSet->x *= 1.0f + (AfterClip.x / AfterClip.w);
    VecToSet->y *= 1.0f - (AfterClip.y / AfterClip.w);

    VecToSet->x = std::clamp(VecToSet->x, 0.f, screenSize.x);
    VecToSet->y = std::clamp(VecToSet->y, 0.f, screenSize.y);

    return true;

}
*/
static Vector Find_Biggest(Vector v1, Vector v2) {
    return Vector((v2 - v1) / 100);
}

static bool worldToScreenClip(Vector in, Vector in2, ImVec2& out, ImVec2& out2) {
    Vector VecToClip;
    Vector VecToKeep;
    ImVec2* VecToSet;
    ImVec2 VecToNotSet;

    int indicator = 0; // GHETTO AF. 1 = in1, 2 = in2, 3 = both.

    if (!worldToScreen(in, out)) {
        indicator++;
    }
    if (!worldToScreen(in2, out2)) {
        indicator += 2;
    }


    switch (indicator) {
    case 0:
        return true;
    case 1:
        VecToClip = in;
        VecToSet = &out;
        VecToKeep = in2;
        VecToNotSet = out2;
        break;
    case 2:
        VecToClip = in2;
        VecToSet = &out2;
        VecToKeep = in;
        VecToNotSet = out;
        break;
    case 3:
    default:
        return false;
    }

    Vector Incer = Find_Biggest(VecToKeep, VecToClip);

    for (float i = 0.f; i < 100.f; i+=1.f) {
        //VecToClip -= Incer;
        if (worldToScreen((VecToClip - (Incer * i)), *VecToSet)) {
            return true;
        }
    }


    return false;


}


static bool getPoints(StreamProofESP::DrawItem Object, Vector& Start, Vector& End) noexcept
{
    Start = Object.StartPos;
    End = Object.EndPos;

    switch (Object.BoundType) {
        case 0:
            return false;
            break; // not needed, but it's for piece of mind.
        case 2:
            if (!Object.boundendEntity || !Object.boundendEntity->isAlive() || Object.boundendEntity->isDormant())
                return true;
            if (Object.boundendEntity->isPlayer()) {
                Vector MidPoint;
                MidPoint = Object.boundendEntity->getEyePosition();
                MidPoint.z /= Object.boundendEntity->origin().z;
                Start = MidPoint;
            }
            else {
                const auto collideable = Object.boundendEntity->getCollideable();
                Vector obbMins = collideable->obbMins();
                Vector obbMaxs = collideable->obbMaxs();
                Start = (obbMins + obbMaxs) / 2;
            }    
        case 1:
            if (!Object.boundstartEntity || Object.boundstartEntity->isDormant() || !Object.boundstartEntity->isAlive())
                return true;
            if (Object.boundstartEntity->isPlayer()) {
                Vector MidPoint;
                MidPoint = Object.boundstartEntity->getEyePosition();
                MidPoint.z += Object.boundstartEntity->origin().z;
                MidPoint.z /= 2;
                End = MidPoint;
            }
            else {
                const auto collideable = Object.boundstartEntity->getCollideable();
                Vector obbMins = collideable->obbMins();
                Vector obbMaxs = collideable->obbMaxs();
                End = (obbMins + obbMaxs) / 2;
            }
            return false;
            break;
        default:
            return true;
    }
}


static unsigned int GetColor(StreamProofESP::DrawItem Object) {
    ColorA colorTMP = Object.Color;

    if (Object.Fade) {
        colorTMP.color[3] = 1.0f * (((Object.creation_time + Object.exist_time) - memory->globalVars->currenttime) / Object.exist_time);
    }

    return Helpers::calculateColor(colorTMP);
}

static int customDrawSkeleton(StreamProofESP::DrawItem& Object) {

    if (memory->globalVars->currenttime > (Object.creation_time + Object.exist_time))
        return 2;

    //if (Object.invalid || !Object.boundendEntity || !Object.boundendEntity->isAlive() || Object.boundendEntity->isDormant())
    //    return true;

    if (Object.invalid)
        return 1;

    std::vector<std::pair<ImVec2, ImVec2>> points, shadowPoints;

    const auto color = GetColor(Object);

    for (const auto& [bone, parent] : Object.bones) {
        ImVec2 bonePoint;
        if (!worldToScreen(bone, bonePoint))
            continue;

        ImVec2 parentPoint;
        if (!worldToScreen(parent, parentPoint))
            continue;

        points.emplace_back(bonePoint, parentPoint);
        shadowPoints.emplace_back(bonePoint + ImVec2{ 1.0f, 1.0f }, parentPoint + ImVec2{ 1.0f, 1.0f });
    }

    for (const auto& [bonePoint, parentPoint] : shadowPoints)
        drawList->AddLine(bonePoint, parentPoint, color, Object.thickness);

    for (const auto& [bonePoint, parentPoint] : points)
        drawList->AddLine(bonePoint, parentPoint, color, Object.thickness);

}

static int customDrawBox(StreamProofESP::DrawItem& Object) {
    if (memory->globalVars->currenttime > (Object.creation_time + Object.exist_time))
        return 2;

    if (Object.invalid)
        return true;

    Vector CenterPoint;
    Vector TMP;
    if (getPoints(Object, TMP, CenterPoint))
        return 1;



    Vector box[4];

    float h = Object.height / 2.f;
    float w = Object.width / 2.f;

    box[0] = { CenterPoint.x + w, CenterPoint.y + w, 0 };
    box[1] = { CenterPoint.x + w, CenterPoint.y - w, 0 };
    box[2] = { CenterPoint.x - w, CenterPoint.y - w, 0 };
    box[3] = { CenterPoint.x - w, CenterPoint.y + w, 0 };

    auto color = GetColor(Object);

    for (int b = -1; b < 2; b += 2) {
        for (int i = 0; i < 4; i++) {
            Vector Start = { box[i].x, box[i].y, CenterPoint.z + (h * b) };

            Vector End;
            if (i == 3) {
                End = { box[0].x, box[0].y, CenterPoint.z + (h * b) };
            }
            else {
                End = { box[i+1].x, box[i+1].y, CenterPoint.z + (h * b) };
            }
            ImVec2 StartP;
            ImVec2 EndP;

            if (!worldToScreen(Start, StartP))
                continue;

            if (!worldToScreen(End, EndP))
                continue;

            drawList->AddLine(StartP, EndP, color, Object.thickness);
        }
    }
    for (int i = 0; i < 4; i++) {
        Vector Start = { box[i].x, box[i].y, CenterPoint.z + (h * -1) };
        Vector End = { box[i].x, box[i].y, CenterPoint.z + (h * 1) };
        ImVec2 StartP;
        ImVec2 EndP;
        if (!worldToScreen(Start, StartP))
            continue;
        if (!worldToScreen(End, EndP))
            continue;
        drawList->AddLine(StartP, EndP, color, Object.thickness);
    }







}

void StreamProofESP::SetupCurrentMatrix(DrawItem& Object, Entity* entity) {

    const auto model = entity->getModel();
    if (!model) {
        Object.invalid = true;
        return;
    }

    const auto studioModel = interfaces->modelInfo->getStudioModel(model);
    if (!studioModel) {
        Object.invalid = true;
        return;
    }


    if (!entity->setupBones(Object.matrix, MAXSTUDIOBONES, BONE_USED_BY_HITBOX, memory->globalVars->currenttime)) {
        Object.invalid = true;
        return;
    }

    Object.bones.reserve(20);

    for (int i = 0; i < studioModel->numBones; ++i) {
        const auto bone = studioModel->getBone(i);

        if (!bone || bone->parent == -1 || !(bone->flags & BONE_USED_BY_HITBOX))
            continue;

        Object.bones.emplace_back(Object.matrix[i].origin(), Object.matrix[bone->parent].origin());
    }
}




static int customDrawLine(StreamProofESP::DrawItem Object) {

    if (memory->globalVars->currenttime > (Object.creation_time + Object.exist_time))
        return 2;

    Vector StartPos;
    Vector EndPos;

    if (getPoints(Object, StartPos, EndPos))
        return 1;

    ImVec2 Start;
    ImVec2 End;

    
    if (!worldToScreen(StartPos, Start)) {   
        return 1;
    }
    if (!worldToScreen(EndPos, End)) {
        return 1;
    }
    
    /*
    if (!worldToScreenClip(StartPos, EndPos, Start, End)) {
        return 1;
    }
    */
    auto color = GetColor(Object);

    drawList->AddLine(Start, End, color, Object.thickness);
}

void StreamProofESP::DrawESPItemList() noexcept
{
    if (!localPlayer || !localPlayer->isAlive() || localPlayer->isDormant())
        return;

    if (ESPItemList.empty())
        return;


    for (int i = 0; i < ESPItemList.size(); i++) {
        DrawItem *Object = &ESPItemList.at(i);

        if (Object->creation_time == 0.0f) {
            Object->creation_time = memory->globalVars->currenttime;
        }
        int Ret = 0;
        switch (Object->type) {
            case LINE:
                Ret = customDrawLine(*Object);
                break;
            case BOX:
                Ret = customDrawBox(*Object);
                break;
            case SKELETON:
                Ret = customDrawSkeleton(*Object);
                break;
            default:
                break;
        }
        if (Ret == 2) {
            std::swap(ESPItemList[i], ESPItemList.back());
            ESPItemList.pop_back();
        }

    }
}






static constexpr auto operator-(float sub, const std::array<float, 3>& a) noexcept
{
    return Vector{ sub - a[0], sub - a[1], sub - a[2] };
}

struct BoundingBox {
private:
    bool valid;
public:
    ImVec2 min, max;
    std::array<ImVec2, 8> vertices;

    BoundingBox(const Vector& mins, const Vector& maxs, const std::array<float, 3>& scale, const matrix3x4* matrix = nullptr) noexcept
    {
        min.y = min.x = std::numeric_limits<float>::max();
        max.y = max.x = -std::numeric_limits<float>::max();

        const auto scaledMins = mins + (maxs - mins) * 2 * (0.25f - scale);
        const auto scaledMaxs = maxs - (maxs - mins) * 2 * (0.25f - scale);

        for (int i = 0; i < 8; ++i) {
            const Vector point{ i & 1 ? scaledMaxs.x : scaledMins.x,
                                i & 2 ? scaledMaxs.y : scaledMins.y,
                                i & 4 ? scaledMaxs.z : scaledMins.z };

            if (!worldToScreen(matrix ? point.transform(*matrix) : point, vertices[i])) {
                valid = false;
                return;
            }

            min.x = std::min(min.x, vertices[i].x);
            min.y = std::min(min.y, vertices[i].y);
            max.x = std::max(max.x, vertices[i].x);
            max.y = std::max(max.y, vertices[i].y);
        }
        valid = true;
    }

    BoundingBox(const BaseData& data, const std::array<float, 3>& scale) noexcept : BoundingBox{ data.obbMins, data.obbMaxs, scale, &data.coordinateFrame } {}
    BoundingBox(const Vector& center) noexcept : BoundingBox{ center - 2.0f, center + 2.0f, { 0.25f, 0.25f, 0.25f } } {}

    operator bool() const noexcept
    {
        return valid;
    }
};



static void addLineWithShadow(const ImVec2& p1, const ImVec2& p2, ImU32 col, float thickness) noexcept
{
    drawList->AddLine(p1 + ImVec2{ 1.0f, 1.0f }, p2 + ImVec2{ 1.0f, 1.0f }, col & IM_COL32_A_MASK, thickness);
    drawList->AddLine(p1, p2, col, thickness);
}

#include "../SDK/LocalPlayer.h"
#include "../Interfaces.h"
#include "../SDK/Entity.h"

void StreamProofESP::bulletTracer(GameEvent* event) noexcept {
    if (!config->visuals.espbulletTracers.enabled || !interfaces->engine->isInGame() || !localPlayer || !localPlayer->isAlive() || localPlayer->isDormant())
        return;

    auto player = interfaces->entityList->getEntity(interfaces->engine->getPlayerForUserID(event->getInt("userid")));

    if (!player || !localPlayer)
        return;

    if (!(player == localPlayer.get()))
        return;



    Vector position;
    position.x = event->getFloat("x");
    position.y = event->getFloat("y");
    position.z = event->getFloat("z");

    

    ColorA color;
    color.color[0] = config->visuals.espbulletTracers.color[0];
    color.color[1] = config->visuals.espbulletTracers.color[1];
    color.color[2] = config->visuals.espbulletTracers.color[2];
    color.color[3] = 1.0f;


    //ESPItemList
    DrawItem Line;
    Line.StartPos = position;
    Line.EndPos = position;
    Line.exist_time = 20.0f;
    Line.Fade = true;
    Line.type = LINE;
    Line.thickness = 3.0f;
    Line.Color = color;

    DrawItem Box;
    Box.EndPos = position;
    Box.StartPos = position;
    Box.exist_time = 5.0f;
    Box.height = 5.0f;
    Box.width = 5.0f;
    Box.Fade = true;
    Box.type = BOX;
    Box.thickness = 1.0f;
    Box.Color = color;

    //ESPItemList.push_back(Line);
    ESPItemList.push_back(Box);
    
    return;
}


static void renderBox(const BoundingBox& bbox, const Box& config) noexcept
{
    if (!config.enabled)
        return;

    const ImU32 color = Helpers::calculateColor(config);

    switch (config.type) {
    case Box::_2d:
        drawList->AddRect(bbox.min + ImVec2{ 1.0f, 1.0f }, bbox.max + ImVec2{ 1.0f, 1.0f }, color & IM_COL32_A_MASK, config.rounding, ImDrawCornerFlags_All, config.thickness);
        drawList->AddRect(bbox.min, bbox.max, color, config.rounding, ImDrawCornerFlags_All, config.thickness);
        break;
    case Box::_2dCorners:
        addLineWithShadow(bbox.min, { bbox.min.x, bbox.min.y * 0.75f + bbox.max.y * 0.25f }, color, config.thickness);
        addLineWithShadow(bbox.min, { bbox.min.x * 0.75f + bbox.max.x * 0.25f, bbox.min.y }, color, config.thickness);

        addLineWithShadow({ bbox.max.x, bbox.min.y }, { bbox.max.x * 0.75f + bbox.min.x * 0.25f, bbox.min.y }, color, config.thickness);
        addLineWithShadow({ bbox.max.x, bbox.min.y }, { bbox.max.x, bbox.min.y * 0.75f + bbox.max.y * 0.25f }, color, config.thickness);

        addLineWithShadow({ bbox.min.x, bbox.max.y }, { bbox.min.x, bbox.max.y * 0.75f + bbox.min.y * 0.25f }, color, config.thickness);
        addLineWithShadow({ bbox.min.x, bbox.max.y }, { bbox.min.x * 0.75f + bbox.max.x * 0.25f, bbox.max.y }, color, config.thickness);

        addLineWithShadow(bbox.max, { bbox.max.x * 0.75f + bbox.min.x * 0.25f, bbox.max.y }, color, config.thickness);
        addLineWithShadow(bbox.max, { bbox.max.x, bbox.max.y * 0.75f + bbox.min.y * 0.25f }, color, config.thickness);
        break;
    case Box::_3d:
        // two separate loops to make shadows not overlap normal lines
        for (int i = 0; i < 8; ++i) {
            for (int j = 1; j <= 4; j <<= 1) {
                if (!(i & j))
                    drawList->AddLine(bbox.vertices[i] + ImVec2{ 1.0f, 1.0f }, bbox.vertices[i + j] + ImVec2{ 1.0f, 1.0f }, color & IM_COL32_A_MASK, config.thickness);
            }
        }
        for (int i = 0; i < 8; ++i) {
            for (int j = 1; j <= 4; j <<= 1) {
                if (!(i & j))
                    drawList->AddLine(bbox.vertices[i], bbox.vertices[i + j], color, config.thickness);
            }
        }
        break;
    case Box::_3dCorners:
        // two separate loops to make shadows not overlap normal lines
        for (int i = 0; i < 8; ++i) {
            for (int j = 1; j <= 4; j <<= 1) {
                if (!(i & j)) {
                    drawList->AddLine(bbox.vertices[i] + ImVec2{ 1.0f, 1.0f }, ImVec2{ bbox.vertices[i].x * 0.75f + bbox.vertices[i + j].x * 0.25f, bbox.vertices[i].y * 0.75f + bbox.vertices[i + j].y * 0.25f } + ImVec2{ 1.0f, 1.0f }, color & IM_COL32_A_MASK, config.thickness);
                    drawList->AddLine(ImVec2{ bbox.vertices[i].x * 0.25f + bbox.vertices[i + j].x * 0.75f, bbox.vertices[i].y * 0.25f + bbox.vertices[i + j].y * 0.75f } + ImVec2{ 1.0f, 1.0f }, bbox.vertices[i + j] + ImVec2{ 1.0f, 1.0f }, color & IM_COL32_A_MASK, config.thickness);
                }
            }
        }

        for (int i = 0; i < 8; ++i) {
            for (int j = 1; j <= 4; j <<= 1) {
                if (!(i & j)) {
                    drawList->AddLine(bbox.vertices[i], { bbox.vertices[i].x * 0.75f + bbox.vertices[i + j].x * 0.25f, bbox.vertices[i].y * 0.75f + bbox.vertices[i + j].y * 0.25f }, color, config.thickness);
                    drawList->AddLine({ bbox.vertices[i].x * 0.25f + bbox.vertices[i + j].x * 0.75f, bbox.vertices[i].y * 0.25f + bbox.vertices[i + j].y * 0.75f }, bbox.vertices[i + j], color, config.thickness);
                }
            }
        }
        break;
    }
}

static ImVec2 renderText(float distance, float cullDistance, const ColorA& textCfg, const char* text, const ImVec2& pos, bool centered = true, bool adjustHeight = true) noexcept
{
    if (cullDistance && Helpers::units2meters(distance) > cullDistance)
        return { };

    const auto textSize = ImGui::CalcTextSize(text);

    const auto horizontalOffset = centered ? textSize.x / 2 : 0.0f;
    const auto verticalOffset = adjustHeight ? textSize.y : 0.0f;

    const auto color = Helpers::calculateColor(textCfg);
    drawList->AddText({ pos.x - horizontalOffset + 1.0f, pos.y - verticalOffset + 1.0f }, color & IM_COL32_A_MASK, text);
    drawList->AddText({ pos.x - horizontalOffset, pos.y - verticalOffset }, color, text);

    return textSize;
}

static void drawSnapline(const Snapline& config, const ImVec2& min, const ImVec2& max) noexcept
{
    if (!config.enabled)
        return;

    const auto& screenSize = ImGui::GetIO().DisplaySize;

    ImVec2 p1, p2;
    p1.x = screenSize.x / 2;
    p2.x = (min.x + max.x) / 2;

    switch (config.type) {
    case Snapline::Bottom:
        p1.y = screenSize.y;
        p2.y = max.y;
        break;
    case Snapline::Top:
        p1.y = 0.0f;
        p2.y = min.y;
        break;
    case Snapline::Crosshair:
        p1.y = screenSize.y / 2;
        p2.y = (min.y + max.y) / 2;
        break;
    default:
        return;
    }

    drawList->AddLine(p1, p2, Helpers::calculateColor(config), config.thickness);
}

#include "Walkbot/nav_area.h"
#include "Walkbot/nav_file.h"

static void drawNavMesh(nav_mesh::nav_file nav) {

    const auto& screenSize = ImGui::GetIO().DisplaySize;

    ImVec2 p1, p2;

    nav_mesh::vec3_t previous = {0,0,0};
    for (auto& area : *(nav.getNavAreas())) {

        auto center = area.get_center();
        p1.x = screenSize.x / 2;
        p2.x = (center.x + previous.x) / 2;



    }

}

struct FontPush {
    FontPush(const std::string& name, float distance)
    {
        distance *= GameData::local().fov / 90.0f;

        ImGui::PushFont([](const Config::Font& font, float dist) {
            if (dist <= 400.0f)
                return font.big;
            if (dist <= 1000.0f)
                return font.medium;
            return font.tiny;
            }(config->fonts[name], distance));
    }

    ~FontPush()
    {
        ImGui::PopFont();
    }
};

#include "Resolver.h"
#include "../SDK/Surface.h"
#include "../Config.h"
static void renderPlayerBox(const PlayerData& playerData, const Player& Pconfig) noexcept
{
    const BoundingBox bbox{ playerData, Pconfig.box.scale };

    if (!bbox)
        return;

    renderBox(bbox, Pconfig.box);
    
    ImVec2 offsetMins{}, offsetMaxs{};

    FontPush font{ Pconfig.font.name, playerData.distanceToLocal };

    if (Pconfig.name.enabled) {
        const auto nameSize = renderText(playerData.distanceToLocal, Pconfig.textCullDistance, Pconfig.name, playerData.name, { (bbox.min.x + bbox.max.x) / 2, bbox.min.y - 5 });
        offsetMins.y -= nameSize.y + 5;
    }

    if (Pconfig.flashDuration.enabled && playerData.flashDuration > 0.0f) {
        const auto radius = std::max(5.0f - playerData.distanceToLocal / 600.0f, 1.0f);
        ImVec2 flashDurationPos{ (bbox.min.x + bbox.max.x) / 2, bbox.min.y + offsetMins.y - radius * 1.5f };

        const auto color = Helpers::calculateColor(Pconfig.flashDuration);
        drawList->PathArcTo(flashDurationPos + ImVec2{ 1.0f, 1.0f }, radius, IM_PI / 2 - (playerData.flashDuration / 255.0f * IM_PI), IM_PI / 2 + (playerData.flashDuration / 255.0f * IM_PI), 40);
        drawList->PathStroke(color & IM_COL32_A_MASK, false, 0.9f + radius * 0.1f);

        drawList->PathArcTo(flashDurationPos, radius, IM_PI / 2 - (playerData.flashDuration / 255.0f * IM_PI), IM_PI / 2 + (playerData.flashDuration / 255.0f * IM_PI), 40);
        drawList->PathStroke(color, false, 0.9f + radius * 0.1f);

        offsetMins.y -= radius * 2.5f;
    }

    if (Pconfig.weapon.enabled && !playerData.activeWeapon.empty()) {
        const auto weaponTextSize = renderText(playerData.distanceToLocal, Pconfig.textCullDistance, Pconfig.weapon, playerData.activeWeapon.c_str(), { (bbox.min.x + bbox.max.x) / 2, bbox.max.y + 5 }, true, false);
        offsetMaxs.y += weaponTextSize.y + 5.0f;
    }

    drawSnapline(Pconfig.snapline, bbox.min + offsetMins, bbox.max + offsetMaxs);

    
    if (Pconfig.drawMultiPoints) {
            static Vector multiPoints[Multipoints::HITBOX_MAX][Multipoints::MULTIPOINTS_MAX];

            auto activeWeapon = localPlayer->getActiveWeapon();
            if (!activeWeapon)
                return;
            auto weaponIndex = getWeaponIndex(activeWeapon->itemDefinitionIndex2()); // get weapon index
            if (!weaponIndex) // no weapon index? then exit
                return;

            if (Multipoints::retrieveAll(interfaces->entityList->getEntity(playerData.entityindx), config->aimbot[weaponIndex].multidistance , Resolver::PlayerRecords[playerData.entityindx].multiExpan, multiPoints)) {

                for (int hitBox = Multipoints::HITBOX_START; hitBox < Multipoints::HITBOX_MAX; hitBox++) {
                    for (int multiPoint = Multipoints::MULTIPOINTS_START; multiPoint < Multipoints::MULTIPOINTS_MAX; multiPoint++) {
                        ImVec2 points;

                        if (worldToScreen(multiPoints[hitBox][multiPoint], points)) {
                            int color = Helpers::calculateColor(Pconfig.flashDuration);
                            const auto radius = std::max(5.0f - playerData.distanceToLocal / 600.0f, 1.0f);
                            drawList->AddCircleFilled(points, radius, color & IM_COL32_A_MASK);
                        }
                        if (!config->aimbot[weaponIndex].multiincrease) {
                            if (multiPoint > 12)
                                break;
                        }
                            //interfaces->surface->drawOutlinedCircle(points.x, points.y, 1, 8);
                    }
                }
            }
        }
        
    }



static void renderWeaponBox(const WeaponData& weaponData, const Weapon& config) noexcept
{
    const BoundingBox bbox{ weaponData, config.box.scale };

    if (!bbox)
        return;

    renderBox(bbox, config.box);
    drawSnapline(config.snapline, bbox.min, bbox.max);

    FontPush font{ config.font.name, weaponData.distanceToLocal };

    if (config.name.enabled && !weaponData.displayName.empty()) {
        renderText(weaponData.distanceToLocal, config.textCullDistance, config.name, weaponData.displayName.c_str(), { (bbox.min.x + bbox.max.x) / 2, bbox.min.y - 5 });
    }

    if (config.ammo.enabled && weaponData.clip != -1) {
        const auto text{ std::to_string(weaponData.clip) + " / " + std::to_string(weaponData.reserveAmmo) };
        renderText(weaponData.distanceToLocal, config.textCullDistance, config.ammo, text.c_str(), { (bbox.min.x + bbox.max.x) / 2, bbox.max.y + 5 }, true, false);
    }
}

static void renderEntityBox(const BaseData& entityData, const char* name, const Shared& config) noexcept
{
    const BoundingBox bbox{ entityData, config.box.scale };

    if (!bbox)
        return;

    renderBox(bbox, config.box);
    drawSnapline(config.snapline, bbox.min, bbox.max);

    FontPush font{ config.font.name, entityData.distanceToLocal };

    if (config.name.enabled)
        renderText(entityData.distanceToLocal, config.textCullDistance, config.name, name, { (bbox.min.x + bbox.max.x) / 2, bbox.min.y - 5 });
}

static void drawProjectileTrajectory(const Trail& config, const std::vector<std::pair<float, Vector>>& trajectory) noexcept
{
    if (!config.enabled)
        return;

    std::vector<ImVec2> points, shadowPoints;

    const auto color = Helpers::calculateColor(config);

    for (const auto& [time, point] : trajectory) {
        if (ImVec2 pos; time + config.time >= memory->globalVars->realtime && worldToScreen(point, pos)) {
            if (config.type == Trail::Line) {
                points.push_back(pos);
                shadowPoints.push_back(pos + ImVec2{ 1.0f, 1.0f });
            } else if (config.type == Trail::Circles) {
                drawList->AddCircle(pos, 3.5f - point.distTo(GameData::local().origin) / 700.0f, color, 12, config.thickness);
            } else if (config.type == Trail::FilledCircles) {
                drawList->AddCircleFilled(pos, 3.5f - point.distTo(GameData::local().origin) / 700.0f, color);
            }
        }
    }

    if (config.type == Trail::Line) {
        drawList->AddPolyline(shadowPoints.data(), shadowPoints.size(), color & IM_COL32_A_MASK, false, config.thickness);
        drawList->AddPolyline(points.data(), points.size(), color, false, config.thickness);
    }
}

static void drawPlayerSkeleton(const ColorToggleThickness& config, const std::vector<std::pair<Vector, Vector>>& bones) noexcept
{
    if (!config.enabled)
        return;

    const auto color = Helpers::calculateColor(config);

    std::vector<std::pair<ImVec2, ImVec2>> points, shadowPoints;

    for (const auto& [bone, parent] : bones) {
        ImVec2 bonePoint;
        if (!worldToScreen(bone, bonePoint))
            continue;

        ImVec2 parentPoint;
        if (!worldToScreen(parent, parentPoint))
            continue;

        points.emplace_back(bonePoint, parentPoint);
        shadowPoints.emplace_back(bonePoint + ImVec2{ 1.0f, 1.0f }, parentPoint + ImVec2{ 1.0f, 1.0f });
    }

    for (const auto& [bonePoint, parentPoint] : shadowPoints)
        drawList->AddLine(bonePoint, parentPoint, color & IM_COL32_A_MASK, config.thickness);

    for (const auto& [bonePoint, parentPoint] : points)
        drawList->AddLine(bonePoint, parentPoint, color, config.thickness);
}

static bool renderPlayerEsp(const PlayerData& playerData, const Player& playerConfig) noexcept
{
    if (!playerConfig.enabled)
        return false;

    if (playerConfig.audibleOnly && !playerData.audible && !playerConfig.spottedOnly
        || playerConfig.spottedOnly && !playerData.spotted && !(playerConfig.audibleOnly && playerData.audible)) // if both "Audible Only" and "Spotted Only" are on treat them as audible OR spotted
        return true;

    renderPlayerBox(playerData, playerConfig);
    drawPlayerSkeleton(playerConfig.skeleton, playerData.bones);

    if (const BoundingBox headBbox{ playerData.headMins, playerData.headMaxs, playerConfig.headBox.scale })
        renderBox(headBbox, playerConfig.headBox);

    return true;
}

static void renderWeaponEsp(const WeaponData& weaponData, const Weapon& parentConfig, const Weapon& itemConfig) noexcept
{
    const auto& config = itemConfig.enabled ? itemConfig : (parentConfig.enabled ? parentConfig : ::config->streamProofESP.weapons["All"]);
    if (config.enabled) {
        renderWeaponBox(weaponData, config);
    }
}

static void renderEntityEsp(const BaseData& entityData, const std::unordered_map<std::string, Shared>& map, const char* name) noexcept
{
    if (const auto cfg = map.find(name); cfg != map.cend() && cfg->second.enabled) {
        renderEntityBox(entityData, name, cfg->second);
    } else if (const auto cfg = map.find("All"); cfg != map.cend() && cfg->second.enabled) {
        renderEntityBox(entityData, name, cfg->second);
    }
}

static void renderProjectileEsp(const ProjectileData& projectileData, const Projectile& parentConfig, const Projectile& itemConfig, const char* name) noexcept
{
    const auto& config = itemConfig.enabled ? itemConfig : parentConfig;

    if (config.enabled) {
        if (!projectileData.exploded)
            renderEntityBox(projectileData, name, config);



        if (config.trails.enabled) {
            if (projectileData.thrownByLocalPlayer) {
                drawProjectileTrajectory(config.trails.localPlayer, projectileData.trajectory);
                drawProjectileTrajectory(config.trails.localPlayerextra, projectileData.trajectory);
            }
            else if (!projectileData.thrownByEnemy)
                drawProjectileTrajectory(config.trails.allies, projectileData.trajectory);
            else
                drawProjectileTrajectory(config.trails.enemies, projectileData.trajectory);
        }
    }
}

void StreamProofESP::render() noexcept
{
    drawList = ImGui::GetBackgroundDrawList();

    GameData::Lock lock;

   ScreenSize = ImGui::GetIO().DisplaySize;

    DrawESPItemList();

    for (const auto& weapon : GameData::weapons())
        renderWeaponEsp(weapon, config->streamProofESP.weapons[weapon.group], config->streamProofESP.weapons[weapon.name]);

    for (const auto& entity : GameData::entities())
        renderEntityEsp(entity, config->streamProofESP.otherEntities, entity.name);

    for (const auto& lootCrate : GameData::lootCrates()) {
        if (lootCrate.name)
            renderEntityEsp(lootCrate, config->streamProofESP.lootCrates, lootCrate.name);
    }

    for (const auto& projectile : GameData::projectiles())
        renderProjectileEsp(projectile, config->streamProofESP.projectiles["All"], config->streamProofESP.projectiles[projectile.name], projectile.name);

    for (const auto& player : GameData::players()) {
        auto& playerConfig = player.enemy ? config->streamProofESP.enemies : config->streamProofESP.allies;

        if (!renderPlayerEsp(player, playerConfig["All"]))
            renderPlayerEsp(player, playerConfig[player.visible ? "Visible" : "Occluded"]);
    }
}
