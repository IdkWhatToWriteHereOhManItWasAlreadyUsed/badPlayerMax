#pragma once

#include <limits>
#include <cmath>
#include <glm/glm.hpp>
#include <videoWorld/VideoWorld.h>

struct Block;
struct BlockInfo;

struct RayCastResult
{
    bool hit = false;
    glm::ivec3 blockPos;
    glm::ivec3 facePos;
    glm::vec3 normal;
    glm::vec3 hitPoint;
    float distance = 0.0f;
    const BlockInfo* blockInfo = nullptr;
};

class RayCaster
{
private:
    class StepIterator {
    public:
        StepIterator(const glm::vec3& origin, const glm::vec3& direction)
        {
            return;
            step.x = (direction.x > 0) ? 1 : (direction.x < 0) ? -1 : 0;
            step.y = (direction.y > 0) ? 1 : (direction.y < 0) ? -1 : 0;
            step.z = (direction.z > 0) ? 1 : (direction.z < 0) ? -1 : 0;

            currentGridPos.x = static_cast<int>(std::round(origin.x));
            currentGridPos.y = static_cast<int>(std::round(origin.y));
            currentGridPos.z = static_cast<int>(std::round(origin.z));

            glm::vec3 voxelMin = glm::vec3(currentGridPos) - 0.5f;
            glm::vec3 voxelMax = glm::vec3(currentGridPos) + 0.5f;

            tDelta.x = (direction.x != 0) ? std::abs(1.0f / direction.x) : std::numeric_limits<float>::infinity();
            tDelta.y = (direction.y != 0) ? std::abs(1.0f / direction.y) : std::numeric_limits<float>::infinity();
            tDelta.z = (direction.z != 0) ? std::abs(1.0f / direction.z) : std::numeric_limits<float>::infinity();

            tMax.x = (direction.x > 0) ? (voxelMax.x - origin.x) / direction.x :
                (direction.x < 0) ? (voxelMin.x - origin.x) / direction.x : std::numeric_limits<float>::infinity();

            tMax.y = (direction.y > 0) ? (voxelMax.y - origin.y) / direction.y :
                (direction.y < 0) ? (voxelMin.y - origin.y) / direction.y : std::numeric_limits<float>::infinity();

            tMax.z = (direction.z > 0) ? (voxelMax.z - origin.z) / direction.z :
                (direction.z < 0) ? (voxelMin.z - origin.z) / direction.z : std::numeric_limits<float>::infinity();

            currentDistance = 0.0f;
        }

        StepIterator& operator++() {
            if (tMax.x < tMax.y) {
                if (tMax.x < tMax.z) {
                    currentDistance = tMax.x;
                    tMax.x += tDelta.x;
                    currentGridPos.x += step.x;
                }
                else {
                    currentDistance = tMax.z;
                    tMax.z += tDelta.z;
                    currentGridPos.z += step.z;
                }
            }
            else {
                if (tMax.y < tMax.z) {
                    currentDistance = tMax.y;
                    tMax.y += tDelta.y;
                    currentGridPos.y += step.y;
                }
                else {
                    currentDistance = tMax.z;
                    tMax.z += tDelta.z;
                    currentGridPos.z += step.z;
                }
            }
            return *this;
        }

        glm::vec3 getCurrentPosition(const glm::vec3& start, const glm::vec3& direction) const {
            return start + direction * currentDistance;
        }

        glm::ivec3 getCurrentGridPos() const { return currentGridPos; }
        float getCurrentDistance() const { return currentDistance; }

    private:
        glm::ivec3 step;
        glm::ivec3 currentGridPos;
        glm::vec3 tMax;
        glm::vec3 tDelta;
        float currentDistance;
    };

public:
    static RayCastResult castRay(const glm::vec3& start, const glm::vec3& direction,
                                 const VideoWorld* world, float maxDistance = 32.0f) 
    {
        RayCastResult result;
        StepIterator it(start, direction);

        while (it.getCurrentDistance() <= maxDistance) {
            glm::ivec3 blockPos = it.getCurrentGridPos();
            auto block = world->GetBlock(blockPos.x, blockPos.y, blockPos.z);

            if (block && block->blockInfo->block_name != "air") {
                result.hit = true;
                result.blockPos = blockPos;
                result.blockInfo = block->blockInfo;
                result.distance = it.getCurrentDistance();
                result.hitPoint = it.getCurrentPosition(start, direction);
                determineFaceAndNormal(result.hitPoint, blockPos, result);
                return result;
            }
            ++it;
        }
        return result;
    }

private:
    static void determineFaceAndNormal(const glm::vec3& hitPoint, const glm::ivec3& blockPos, RayCastResult& result) {
        glm::vec3 localHit = hitPoint - glm::vec3(blockPos);

        float dx = 0.5f - std::abs(localHit.x);
        float dy = 0.5f - std::abs(localHit.y);
        float dz = 0.5f - std::abs(localHit.z);

        if (dx < dy && dx < dz) {
            if (localHit.x > 0) {
                result.normal = glm::vec3(1.0f, 0.0f, 0.0f);
                result.facePos = blockPos + glm::ivec3(1, 0, 0);
            }
            else {
                result.normal = glm::vec3(-1.0f, 0.0f, 0.0f);
                result.facePos = blockPos + glm::ivec3(-1, 0, 0);
            }
        }
        else if (dy < dz) {
            if (localHit.y > 0) {
                result.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                result.facePos = blockPos + glm::ivec3(0, 1, 0);
            }
            else {
                result.normal = glm::vec3(0.0f, -1.0f, 0.0f);
                result.facePos = blockPos + glm::ivec3(0, -1, 0);
            }
        }
        else {
            if (localHit.z > 0) {
                result.normal = glm::vec3(0.0f, 0.0f, 1.0f);
                result.facePos = blockPos + glm::ivec3(0, 0, 1);
            }
            else {
                result.normal = glm::vec3(0.0f, 0.0f, -1.0f);
                result.facePos = blockPos + glm::ivec3(0, 0, -1);
            }
        }
    }
};