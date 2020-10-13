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
#include "imgui/imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui_internal.h"
#include "Helpers.h"
#include "GameData.h"
#include "Hacks/StreamProofESP.h"
#include "SDK/Math.h"
#include "Debug.h"


static bool worldToScreen(const Vector& in, ImVec2& out) noexcept
{
    const auto& matrix = GameData::toScreenMatrix();

    const auto w = matrix._41 * in.x + matrix._42 * in.y + matrix._43 * in.z + matrix._44;

    out = ImGui::GetIO().DisplaySize / 2.0f;
    out.x *= 1.0f + (matrix._11 * in.x + matrix._12 * in.y + matrix._13 * in.z + matrix._14) / w;
    out.y *= 1.0f - (matrix._21 * in.x + matrix._22 * in.y + matrix._23 * in.z + matrix._24) / w;

    if (w < 0.001f) {
        return false;
    }

    return true;
}


static bool findBounds() {

}


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


    /*
    
        Basically we calculate the multipoints & w2s them. Then we figure out the > X, > Y, and > Z points of the hitbox. If that returns less than zero (out of screen) we default to the old way of 
        finding multipoints (return the points we had). Otherwise, we return many less points.

        This can certainly be done by using crazy matrix math. 
    
    
    
    
    */


    bool retrieveAll(Entity* entity, float expansion, PointScales scales, Multipoint& points, matrix3x4* bonesptr, bool MatrixPassed) {
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

        Vector rotationAxis = { 0, 0, 1 };
        

        matrix3x4 m_tranformationmatrix = entity->toWorldTransform();
        

        for (int hitBox = Multipoints::HITBOX_START; hitBox < (std::min) (hitBoxSetPtr->numHitboxes, (decltype(hitBoxSetPtr->numHitboxes))Multipoints::HITBOX_MAX); hitBox++)
        {
            HitboxPoints point;
            point.HitBox = hitBox;
            StudioBbox* box = hitBoxSetPtr->getHitbox(hitBox);
            float pscale = 1.0f;
            if (box)
            {
                Vector bmin = box->bbMin;
                Vector bmax = box->bbMax;

                //Vector min = box->bbMin;
                //Vector max = box->bbMax;
                if (box->capsuleRadius > 0.0f)
                {
                    //bmin.transform(bones[box->bone]);
                    //bmax.transform(bones[box->bone]);
                    Vector delta = bmax - bmin;
                    float length = delta.length2D();
                    Vector Source = Vector{ 0,0,length / 2 };


                    Vector Center = (bmax + bmin) * .5;
                    point.Points.push_back(Center.transform(bones[box->bone]));
                    Vector Direction = Math::CalcAngle(Center.transform(bones[box->bone]), localPlayer->getEyePosition());
                    
                    float distance_to_edge = box->capsuleRadius / Direction.length2D();

                    Vector end_vec = Source + (Direction * distance_to_edge);

                    if (end_vec.z > length) {
                        Vector retrack_vec = end_vec - length;

                        retrack_vec /= retrack_vec.length2D();

                        end_vec = retrack_vec * box->capsuleRadius;
                        end_vec.z += length;
                    }
                    else if (end_vec.z < 0) {
                        Vector retrack_vec = end_vec;
                        retrack_vec /= retrack_vec.length2D();
                        end_vec = retrack_vec * box->capsuleRadius;
                    }

                    //end_vec.z = Center.z + (bmax.z * (length * box->capsuleRadius * pscale));

                    matrix3x4 space_rot;
                    space_rot = Math::VectorMatrix(Vector{ 0, 0, 1}, space_rot);

                    matrix3x4 space;
                    space_rot = Math::VectorMatrix(delta, space);


                    //end_vec = Math::VectorRotate(end_vec, space_rot);
                    //end_vec = Math::VectorRotate(end_vec, space);
                    //end_vec += max;
                    point.Points.push_back(end_vec.transform(bones[box->bone]));


                    //Center.transform(bones[box->bone]);
                    // https://www.unknowncheats.me/forum/counterstrike-global-offensive/418191-getting-accurate-hitbox.html
                    // https://www.unknowncheats.me/forum/counterstrike-global-offensive/124388-bounding-esp-boxes.html
                    // https://www.unknowncheats.me/forum/counterstrike-global-offensive/410044-getting-accurate-hitbox-capsule.html

                }
                else { // Square


                }
                
            
            }
            points.push_back(point);
            
        }
        return true;

        /*
        
                *----------------------*
                |                      |
                |                      |
                |          GG          |
                |          GG          |
                |                      |
                |                      |
                *----------------------*
                
                
             
        //-----------------------------------------------------------------------------
        // Purpose: Draws a box around an entity
        //-----------------------------------------------------------------------------

        void NDebugOverlay::EntityBounds( const CBaseEntity *pEntity, int r, int g, int b, int a, float flDuration )
        {
	        const CCollisionProperty *pCollide = pEntity->CollisionProp();
	        // Draw the base OBB for the object (default color is orange)
	        BoxAngles( pCollide->GetCollisionOrigin(), pCollide->OBBMins(), pCollide->OBBMaxs(), pCollide->GetCollisionAngles(), r, g, b, a, flDuration );

	        // This is the axis of rotation in world space
	        Vector rotationAxisWs(0,0,1);
	        const float rotationAngle = gpGlobals->curtime*10; // 10 degrees per second animated rotation
	        //const float rotationAngle = 45; // degrees, Source's convention is that positive rotation is counter-clockwise
	

	        // Example 1: Applying the rotation in the local space of the entity

	        // Compute rotation axis in entity local space
	        // Compute the transform as a matrix so we can concatenate it with the entity's current transform
	        Vector rotationAxisLs;

	        // The matrix maps vectors from entity space to world space, since we have a world space 
	        //vector that we want in entity space we use the inverse operator VectorIRotate instead of VectorRotate
            //    Note, you could also invert the matrix and use VectorRotate instead 
                VectorIRotate(rotationAxisWs, pEntity->EntityToWorldTransform(), rotationAxisLs);

                // Build a transform that rotates around that axis in local space by the angle
                // If there were an AxisAngleMatrix() routine we could use that directly, but there isn't
                // So convert to a quaternion first, then a matrix 
                Quaternion q;

                // NOTE: Assumes axis is a unit vector, non-unit vectors will bias the resulting rotation angle (but not the axis)
                AxisAngleQuaternion(rotationAxisLs, rotationAngle, q);

                // Convert to a matrix
                matrix3x4_t xform;
                QuaternionMatrix(q, vec3_origin, xform);

                // Apply the rotation to the entity input space (local)
                matrix3x4_t localToWorldMatrix;
                ConcatTransforms(pEntity->EntityToWorldTransform(), xform, localToWorldMatrix);

                // Extract the compound rotation as a QAngle
                QAngle localAngles;
                MatrixAngles(localToWorldMatrix, localAngles);

                // Draw the rotated box in blue
                BoxAngles(pCollide->GetCollisionOrigin(), pCollide->OBBMins(), pCollide->OBBMaxs(), localAngles, 0, 0, 255, a, flDuration);


                {

                    // Example 2: Applying the rotation in world space directly

                    // Build a transform that rotates around that axis in world space by the angle
                    //NOTE: Add ten degrees so the boxes are separately visible
                   // Then compute the transform as a matrix so we can concatenate it with the entity's current transform 
                    Quaternion q;
                    AxisAngleQuaternion(rotationAxisWs, rotationAngle + 10, q);

                    // Convert to a matrix
                    matrix3x4_t xform;
                    QuaternionMatrix(q, vec3_origin, xform);

                    // Apply the rotation to the entity output space (world)
                    matrix3x4_t localToWorldMatrix;
                    ConcatTransforms(xform, pEntity->EntityToWorldTransform(), localToWorldMatrix);

                    // Extract the compound rotation as a QAngle
                    QAngle localAngles;
                    MatrixAngles(localToWorldMatrix, localAngles);

                    // Draw the rotated + 10 box in yellow
                    BoxAngles(pCollide->GetCollisionOrigin(), pCollide->OBBMins(), pCollide->OBBMaxs(), localAngles, 255, 255, 0, a, flDuration);
                }
            }

               
    */













    }




    




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

