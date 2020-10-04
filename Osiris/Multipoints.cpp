#include "Interfaces.h"
#include "Memory.h"

#include "SDK/Entity.h"
#include "SDK/Vector.h"
#include "SDK/matrix3x4.h"
#include "SDK/LocalPlayer.h"
#include "SDK/GlobalVars.h"
#include "SDK/ModelInfo.h"
#include "Memory.h"
#include "MemAlloc.h"
#include "SDK/GlobalVars.h"
#include "Multipoints.h"

namespace Multipoints
{
    bool retrieveAll(Entity* entity, float multiPointsExpansion, float secondExpan, Vector(&multiPoints)[Multipoints::HITBOX_MAX][Multipoints::MULTIPOINTS_MAX], matrix3x4* bonesptr, bool MatrixPassed)
    {
        if (!localPlayer)
            return false;

        if (!entity)
            return false;

        static matrix3x4 bones[256];


        if (!MatrixPassed || !bonesptr || config->debug.forcesetupBones) {
            if (!entity->setupBones(bones, ARRAYSIZE(bones), 256, memory->globalVars->currenttime)) {
                return false;
            }
        }
        else {
            memcpy(bones, bonesptr, sizeof(matrix3x4) * 256);
        }

        const Model* model = entity->getModel();

        if (!model)
            return false;

        StudioHdr* hdr = interfaces->modelInfo->getStudioModel(model);

        if (!hdr)
            return false;

        int hitBoxSet = entity->hitboxSet();

        if (hitBoxSet < 0)
            return false;

        StudioHitboxSet* hitBoxSetPtr = hdr->getHitboxSet(hitBoxSet);

        if (!hitBoxSetPtr)
            return false;

        for (int hitBox = Multipoints::HITBOX_START; hitBox < (std::min) (hitBoxSetPtr->numHitboxes, (decltype(hitBoxSetPtr->numHitboxes))Multipoints::HITBOX_MAX); hitBox++)
        {
            StudioBbox* box = hitBoxSetPtr->getHitbox(hitBox);

            if (box)
            {
                Vector min = box->bbMin;
                Vector max = box->bbMax;
                Vector minexpan = box->bbMin;
                Vector maxexpan = box->bbMax;
                if (box->capsuleRadius > 0.0f)
                {
                    min -= Vector{ box->capsuleRadius * multiPointsExpansion, box->capsuleRadius * multiPointsExpansion, box->capsuleRadius * multiPointsExpansion, };
                    max += Vector{ box->capsuleRadius * multiPointsExpansion, box->capsuleRadius * multiPointsExpansion, box->capsuleRadius * multiPointsExpansion, };

                    minexpan -= Vector{ box->capsuleRadius * secondExpan, box->capsuleRadius * secondExpan, box->capsuleRadius * secondExpan, };
                    maxexpan += Vector{ box->capsuleRadius * secondExpan, box->capsuleRadius * secondExpan, box->capsuleRadius * secondExpan, };
                };

                static Vector locations[Multipoints::MULTIPOINTS_MAX];

                locations[Multipoints::MULTIPOINTS_CENTER] = (min + max) * 0.5f;

                locations[1] = Vector{min.x,                    ((min.y + max.y) * 0.5f),   ((min.z + max.z) * .5f) };
                locations[2] = Vector{((min.x + max.x) * 0.5f)  , max.y,                  ((min.z + max.z) * .5f) };
                locations[3] = Vector{max.x,                    ((min.y + max.y) * 0.5f),   ((min.z + max.z) * .5f) };
                locations[4] = Vector{((min.x + max.x) * 0.5f)  , min.y,                  ((min.z + max.z) * .5f) };

                locations[5] = Vector{ min.x, min.y, min.z, };
                locations[6] = Vector{ min.x, min.y, max.z, };

                locations[7] = Vector{ max.x, max.y, max.z, };
                locations[8] = Vector{ max.x, max.y, min.z, };


                locations[9] = Vector{ max.x, min.y, min.z, };
                locations[10] = Vector{ max.x, min.y, max.z, };


                locations[11] = Vector{ min.x, max.y, min.z, };
                locations[12] = Vector{ min.x, max.y, max.z, };


                locations[13] = Vector{ minexpan.x, minexpan.y,minexpan.z, };
                locations[14] = Vector{ minexpan.x, minexpan.y, maxexpan.z, };

                locations[15] = Vector{ maxexpan.x, maxexpan.y, maxexpan.z, };
                locations[16] = Vector{ maxexpan.x, maxexpan.y, minexpan.z, };

                locations[17] = Vector{ maxexpan.x, minexpan.y, minexpan.z, };
                locations[18] = Vector{ maxexpan.x, minexpan.y, maxexpan.z, };

                locations[19] = Vector{ minexpan.x, maxexpan.y, minexpan.z, };
                locations[20] = Vector{ minexpan.x, maxexpan.y, maxexpan.z, };

                for (int multiPoint = Multipoints::MULTIPOINTS_START; multiPoint < Multipoints::MULTIPOINTS_MAX; multiPoint++)
                {
                    if (multiPoint > Multipoints::MULTIPOINTS_START)
                    {
                        locations[multiPoint] = (((locations[multiPoint] + locations[Multipoints::MULTIPOINTS_CENTER]) * 0.5f) + locations[multiPoint]) * 0.5f;
                    };

                    multiPoints[hitBox][multiPoint] = locations[multiPoint].transform(bones[box->bone]);
                };
            };
        };

        return true;
    };

    bool retrieveOne(Entity* entity, float multiPointsExpansion, Vector(&multiPoints)[Multipoints::MULTIPOINTS_MAX], int desiredHitBox)
    {
        if (desiredHitBox < Multipoints::HITBOX_START || desiredHitBox > Multipoints::HITBOX_LAST_ENTRY)
            return false;

        if (!localPlayer)
            return false;

        if (!entity)
            return false;

        static matrix3x4 bones[256];

        if (!entity->setupBones(bones, ARRAYSIZE(bones), 256, memory->globalVars->currenttime))
            return false;

        const Model* model = entity->getModel();

        if (!model)
            return false;

        StudioHdr* hdr = interfaces->modelInfo->getStudioModel(model);

        if (!hdr)
            return false;

        int hitBoxSet = entity->hitboxSet();

        if (hitBoxSet < 0)
            return false;

        StudioHitboxSet* hitBoxSetPtr = hdr->getHitboxSet(hitBoxSet);

        if (!hitBoxSetPtr)
            return false;

        StudioBbox* box = hitBoxSetPtr->getHitbox(desiredHitBox);

        if (!box)
            return false;

        Vector min = box->bbMin;
        Vector max = box->bbMax;

        if (box->capsuleRadius > 0.0f)
        {
            min -= Vector{ box->capsuleRadius * multiPointsExpansion, box->capsuleRadius * multiPointsExpansion, box->capsuleRadius * multiPointsExpansion, };
            max += Vector{ box->capsuleRadius * multiPointsExpansion, box->capsuleRadius * multiPointsExpansion, box->capsuleRadius * multiPointsExpansion, };
        };

        static Vector locations[Multipoints::MULTIPOINTS_MAX];

        locations[Multipoints::MULTIPOINTS_CENTER] = (min + max) * 0.5f;

        locations[1] = Vector{ min.x, min.y, min.z, };
        locations[2] = Vector{ min.x, max.y, min.z, };
        locations[3] = Vector{ max.x, max.y, min.z, };
        locations[4] = Vector{ max.x, min.y, min.z, };
        locations[5] = Vector{ max.x, max.y, max.z, };
        locations[6] = Vector{ min.x, max.y, max.z, };
        locations[7] = Vector{ min.x, min.y, max.z, };
        locations[8] = Vector{ max.x, min.y, max.z, };

        for (int multiPoint = Multipoints::MULTIPOINTS_START; multiPoint < Multipoints::MULTIPOINTS_MAX; multiPoint++)
        {
            if (multiPoint > Multipoints::MULTIPOINTS_START)
            {
                locations[multiPoint] = (((locations[multiPoint] + locations[Multipoints::MULTIPOINTS_CENTER]) * 0.5f) + locations[multiPoint]) * 0.5f;
            };

            multiPoints[multiPoint] = locations[multiPoint].transform(bones[box->bone]);
        };

        return true;
    };
};

