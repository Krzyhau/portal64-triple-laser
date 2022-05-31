#include "collision_scene.h"

#include "math/mathf.h"
#include "gjk.h"
#include "epa.h"

struct CollisionScene gCollisionScene;

void collisionSceneInit(struct CollisionScene* scene, struct CollisionObject* quads, int quadCount, struct World* world) {
    scene->quads = quads;
    scene->quadCount = quadCount;

    scene->world = world;

    scene->dynamicObjectCount = 0;
    scene->portalTransforms[0] = NULL;
    scene->portalTransforms[1] = NULL;
}

int mergeColliderList(short* a, int aCount, short* b, int bCount, short* output) {
    int aIndex = 0;
    int bIndex = 0;
    int result = 0;

    while (aIndex < aCount && bIndex < bCount) {
        if (a[aIndex] == b[bIndex]) {
            output[result] = a[aIndex];
            ++result;
            ++aIndex;
            ++bIndex;
        } else if (a[aIndex] < b[bIndex]) {
            output[result] = a[aIndex];
            ++result;
            ++aIndex;
        } else {
            output[result] = b[bIndex];
            ++result;
            ++bIndex;
        }
    }

    while (aIndex < aCount) {
        output[result] = a[aIndex];
        ++result;
        ++aIndex;
    }
    
    while (bIndex < bCount) {
        output[result] = b[bIndex];
        ++result;
        ++bIndex;
    }

    return result;
}

#define MAX_COLLIDERS   64

#define COLLISION_GRID_CELL_SIZE  4

#define GRID_CELL_X(room, worldX) floorf(((worldX) - room->cornerX) * (1.0f / COLLISION_GRID_CELL_SIZE));
#define GRID_CELL_Z(room, worldZ) floorf(((worldZ) - room->cornerZ) * (1.0f / COLLISION_GRID_CELL_SIZE));

#define GRID_CELL_CONTENTS(room, x, z) (&room->cellContents[(x) * room->spanX + (z)])

int collisionObjectRoomColliders(struct Room* room, struct Box3D* box, short output[MAX_COLLIDERS]) {
    short tmp[MAX_COLLIDERS];

    short* currentSource = tmp;
    short* currentResult = output;
    int result = 0;

    int minX = GRID_CELL_X(room, box->min.x);
    int maxX = GRID_CELL_X(room, box->max.x);

    int minZ = GRID_CELL_Z(room, box->min.z);
    int maxZ = GRID_CELL_Z(room, box->max.z);
    
    for (int x = MAX(minX, 0); x <= maxX && x < room->spanX; ++x) {
        for (int z = MAX(minZ, 0); z <= maxZ && z < room->spanZ; ++z) {
            struct Rangeu16* range = GRID_CELL_CONTENTS(room, x, z);

            result = mergeColliderList(currentSource, result, &room->quadIndices[range->min], range->max - range->min, currentResult);

            if (currentResult == output) {
                currentResult = tmp;
                currentSource = output;
            } else {
                currentResult = output;
                currentSource = tmp;
            }
        }
    }

    if (currentSource != output) {
        mergeColliderList(currentSource, result, NULL, 0, output);
    }

    return result;
}

void collisionObjectCollideWithScene(struct CollisionObject* object, struct CollisionScene* scene, struct ContactSolver* contactSolver) {    
    short colliderIndices[MAX_COLLIDERS];
    int quadCount = collisionObjectRoomColliders(&scene->world->rooms[object->body->currentRoom], &object->boundingBox, colliderIndices);

    for (int i = 0; i < quadCount; ++i) {
        collisionObjectCollideWithQuad(object, &scene->quads[colliderIndices[i]], contactSolver);
    }
}

int collisionSceneFilterPortalContacts(struct ContactConstraintState* contact) {
    int writeIndex = 0;

    for (int readIndex = 0; readIndex < contact->contactCount; ++readIndex) {
        if (collisionSceneIsTouchingPortal(&contact->contacts[readIndex].ra, &contact->normal)) {
            continue;;
        }

        if (readIndex != writeIndex) {
            contact->contacts[writeIndex] = contact->contacts[readIndex];
        }

        ++writeIndex;
    }

    contact->contactCount = writeIndex;

    return writeIndex;
}

void collisionObjectQueryScene(struct CollisionObject* object, struct CollisionScene* scene, void* data, ManifoldCallback callback) {
    CollideWithQuad quadCollider = object->collider->callbacks->collideWithQuad;

    if (!quadCollider) {
        return;
    }

    short colliderIndices[MAX_COLLIDERS];
    int quadCount = collisionObjectRoomColliders(&scene->world->rooms[object->body->currentRoom], &object->boundingBox, colliderIndices);

    struct ContactConstraintState localContact;

    for (int i = 0; i < quadCount; ++i) {
        localContact.contactCount = 0;

        if (quadCollider(object->collider->data, &object->body->transform, scene->quads[colliderIndices[i]].collider->data, &localContact) &&
            collisionSceneFilterPortalContacts(&localContact)) {
            callback(data, &localContact);
        }
    }
}

int collisionSceneIsTouchingSinglePortal(struct Vector3* contactPoint, struct Vector3* contactNormal, struct Transform* portalTransform, int portalIndex) {
    struct Vector3 localPoint;
    transformPointInverse(portalTransform, contactPoint, &localPoint);

    if (fabsf(localPoint.z) > PORTAL_THICKNESS) {
        return 0;
    }

    localPoint.x *= (1.0f / PORTAL_X_RADIUS);
    localPoint.z = 0.0f;

    if (vector3MagSqrd(&localPoint) >= 1.0f) {
        return 0;
    }

    struct Vector3 portalNormal = gZeroVec;
    portalNormal.z = portalIndex ? 1.0f : -1.0f;
    quatMultVector(&portalTransform->rotation, &portalNormal, &portalNormal);

    return vector3Dot(contactNormal, &portalNormal) > 0.0f;
}

int collisionSceneIsTouchingPortal(struct Vector3* contactPoint, struct Vector3* contactNormal) {
    if (!collisionSceneIsPortalOpen()) {
        return 0;
    }

    for (int i = 0; i < 2; ++i) {
        if (collisionSceneIsTouchingSinglePortal(contactPoint, contactNormal, gCollisionScene.portalTransforms[i], i)) {
            return 1;
        }
    }

    return 0;
}

int collisionSceneIsPortalOpen() {
    return gCollisionScene.portalTransforms[0] != NULL && gCollisionScene.portalTransforms[1] != NULL;
}

void collisionSceneRaycastRoom(struct CollisionScene* scene, struct Room* room, struct Ray* ray, struct RaycastHit* hit) {
    int currX = GRID_CELL_X(room, ray->origin.x);
    int currZ = GRID_CELL_Z(room, ray->origin.z);

    float xDirInv = fabsf(ray->dir.x) > 0.00001f ? 1.0f / ray->dir.x : 0.0f;
    float zDirInv = fabsf(ray->dir.z) > 0.00001f ? 1.0f / ray->dir.z : 0.0f;

    // TODO adjust currX and currZ if ray starts outside room

    while (currX >=0 && currX < room->spanX && currZ >= 0 && currZ < room->spanZ) {
        struct Rangeu16* range = GRID_CELL_CONTENTS(room, currX, currZ);

        for (int i = range->min; i < range->max; ++i) {
            struct RaycastHit hitTest;

            if (raycastQuad(&scene->quads[room->quadIndices[i]], ray, hit->distance, &hitTest) && hitTest.distance < hit->distance && vector3Dot(&hitTest.normal, &ray->dir) < 0.0f) {
                hit->at = hitTest.at;
                hit->normal = hitTest.normal;
                hit->distance = hitTest.distance;
                hit->object = hitTest.object;
            }
        }

        int xStep = 0;
        int zStep = 0;
        float cellDistance = hit->distance;

        if (xDirInv != 0.0f) {
            float nextEdge = (currX + (ray->dir.x > 0.0f ? 1 : 0)) * COLLISION_GRID_CELL_SIZE + room->cornerX;
            float distanceCheck = (nextEdge - ray->origin.x) * xDirInv;

            if (distanceCheck < cellDistance) {
                cellDistance = distanceCheck;
                xStep = ray->dir.x > 0.0f ? 1 : -1;
                zStep = 0;
            }
        }

        if (zDirInv != 0.0f) {
            float nextEdge = (currZ + (ray->dir.z > 0.0f ? 1 : 0)) * COLLISION_GRID_CELL_SIZE + room->cornerZ;
            float distanceCheck = (nextEdge - ray->origin.z) * zDirInv;

            if (distanceCheck < cellDistance) {
                cellDistance = distanceCheck;
                xStep = 0;
                zStep = ray->dir.z > 0.0f ? 1 : -1;
            }
        }

        if (!xStep && !zStep) {
            return;
        }

        currX += xStep;
        currZ += zStep;
    }
}

int collisionSceneRaycastDoorways(struct CollisionScene* scene, struct Room* room, struct Ray* ray, float maxDistance, int currentRoom) {
    int nextRoom = -1;

    float roomDistance = maxDistance;

    for (int i = 0; i < room->doorwayCount; ++i) {
        struct RaycastHit hitTest;

        struct Doorway* doorway = &scene->world->doorways[room->doorwayIndices[i]];

        if (raycastQuadShape(&doorway->quad, ray, roomDistance, &hitTest) && hitTest.distance < roomDistance) {
            roomDistance = hitTest.distance;
            // check that the doorway wasn't hit from the wrong side
            int expectedRoom = vector3Dot(&doorway->quad.plane.normal, &hitTest.normal) < 0.0f ? doorway->roomA : doorway->roomB;
            int otherRoom = currentRoom == doorway->roomA ? doorway->roomB : doorway->roomA;

            if (expectedRoom == otherRoom) {
                nextRoom = otherRoom;
            }
        }
    }

    return nextRoom;
}

int collisionSceneRaycast(struct CollisionScene* scene, int roomIndex, struct Ray* ray, float maxDistance, int passThroughPortals, struct RaycastHit* hit) {
    hit->distance = maxDistance;
    hit->throughPortal = NULL;

    int roomsToCheck = 5;

    while (roomsToCheck && roomIndex != -1) {
        struct Room* room = &scene->world->rooms[roomIndex];
        collisionSceneRaycastRoom(scene, room, ray, hit);

        if (hit->distance != maxDistance) {
            hit->roomIndex = roomIndex;
            break;
        }

        int nextRoom = collisionSceneRaycastDoorways(scene, room, ray, hit->distance, roomIndex);

        roomIndex = nextRoom;

        --roomsToCheck;
    }

    for (int i = 0; i < scene->dynamicObjectCount; ++i) {
        struct RaycastHit hitTest;

        struct CollisionObject* object = scene->dynamicObjects[i];

        if (object->collider->callbacks->raycast && 
            object->collider->callbacks->raycast(object, ray, hit->distance, &hitTest) &&
            hitTest.distance < hit->distance) {
            hit->at = hitTest.at;
            hit->normal = hitTest.normal;
            hit->distance = hitTest.distance;
            hit->object = hitTest.object;
            hit->roomIndex = hitTest.roomIndex;
        }
    }

    if (passThroughPortals && 
        hit->distance != maxDistance &&
        collisionSceneIsPortalOpen()) {
        for (int i = 0; i < 2; ++i) {
            if (collisionSceneIsTouchingSinglePortal(&hit->at, &hit->normal, gCollisionScene.portalTransforms[i], i)) {
                struct Transform portalTransform;
                collisionSceneGetPortalTransform(i, &portalTransform);

                struct Ray newRay;

                transformPoint(&portalTransform, &hit->at, &newRay.origin);
                quatMultVector(&portalTransform.rotation, &ray->dir, &newRay.dir);

                struct RaycastHit newHit;

                int result = collisionSceneRaycast(scene, gCollisionScene.portalRooms[1 - i], &newRay, maxDistance - hit->distance, 0, &newHit);

                if (result) {
                    newHit.distance += hit->distance;
                    newHit.throughPortal = gCollisionScene.portalTransforms[i];
                    *hit = newHit;
                }

                return result;
            }
        }
    }

    return hit->distance != maxDistance;
}

void collisionSceneGetPortalTransform(int fromPortal, struct Transform* out) {
    struct Transform inverseA;
    transformInvert(gCollisionScene.portalTransforms[fromPortal], &inverseA);
    transformConcat(gCollisionScene.portalTransforms[1 - fromPortal], &inverseA, out);
}

void collisionSceneAddDynamicObject(struct CollisionObject* object) {
    if (gCollisionScene.dynamicObjectCount < MAX_DYNAMIC_OBJECTS) {
        gCollisionScene.dynamicObjects[gCollisionScene.dynamicObjectCount] = object;
        ++gCollisionScene.dynamicObjectCount;
    }
}

void collisionSceneRemoveDynamicObject(struct CollisionObject* object) {
    int found = 0;

    for (unsigned i = 0; i < gCollisionScene.dynamicObjectCount; ++i) {
        if (object == gCollisionScene.dynamicObjects[i]) {
            found = 1;
        }

        if (found && i + 1 < gCollisionScene.dynamicObjectCount) {
            gCollisionScene.dynamicObjects[i] = gCollisionScene.dynamicObjects[i + 1];
        }
    }

    if (found) {
        --gCollisionScene.dynamicObjectCount;
    }
}

void collisionSceneUpdateDynamics() {
    for (unsigned i = 0; i < gCollisionScene.dynamicObjectCount; ++i) {
        collisionObjectCollideWithScene(gCollisionScene.dynamicObjects[i], &gCollisionScene, &gContactSolver);
    }

    contactSolverSolve(&gContactSolver);

    for (unsigned i = 0; i < gCollisionScene.dynamicObjectCount; ++i) {
        rigidBodyUpdate(gCollisionScene.dynamicObjects[i]->body);
        rigidBodyCheckPortals(gCollisionScene.dynamicObjects[i]->body);
        collisionObjectUpdateBB(gCollisionScene.dynamicObjects[i]);
    }
}

int collisionSceneTestMinkowsiSum(struct CollisionObject* object) {
    if (!object->collider->callbacks->minkowsiSum) {
        return 0;
    }

    struct MinkowsiSumAgainstQuad quadSum;
    quadSum.collisionObject = object;
    struct Simplex simplex;

    short colliderIndices[MAX_COLLIDERS];
    int quadCount = collisionObjectRoomColliders(&gCollisionScene.world->rooms[object->body->currentRoom], &object->boundingBox, colliderIndices);

    for (int i = 0; i < quadCount; ++i) {
        quadSum.quad = gCollisionScene.quads[colliderIndices[i]].collider->data;

        if (box3DHasOverlap(&object->boundingBox, &gCollisionScene.quads[colliderIndices[i]].boundingBox) && gjkCheckForOverlap(&simplex, &quadSum, minkowsiSumAgainstQuadSum, &quadSum.quad->plane.normal)) {
            struct EpaResult result;
            epaSolve(&simplex, &quadSum, minkowsiSumAgainstQuadSum, &result);
            return 1;
        }
    }

    return 0;
}