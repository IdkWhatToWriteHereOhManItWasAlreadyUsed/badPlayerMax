#include "Frustum.h"

void Frustum::update(const glm::mat4& viewProjectionMatrix) 
{
    extractPlanes(viewProjectionMatrix);
}

bool Frustum::isAABBVisible(const glm::vec3& min, const glm::vec3& max) const 
{
    for (int i = 0; i < COUNT; ++i) {
        if (isAABBOutsidePlane(min, max, planes[i])) 
            return false;      
    }
    return true;
}

bool Frustum::isSphereVisible(const glm::vec3& center, float radius) const 
{
    for (int i = 0; i < COUNT; ++i) {
        if (isSphereOutsidePlane(center, radius, planes[i])) {
            return false;
        }
    }
    return true;
}

bool Frustum::isPointVisible(const glm::vec3& point) const 
{
    for (int i = 0; i < COUNT; ++i) 
        if (getDistanceToPlane(point, planes[i]) < 0.0f) 
            return false;
    return true;
}

glm::vec4 Frustum::getPlane(Plane plane) const
{
    return planes[plane];
}

void Frustum::extractPlanes(const glm::mat4& matrix) 
{
    // Left plane
    planes[LEFT] = normalizePlane(glm::vec4(
        matrix[0][3] + matrix[0][0],
        matrix[1][3] + matrix[1][0],
        matrix[2][3] + matrix[2][0],
        matrix[3][3] + matrix[3][0]
    ));

    // Right plane
    planes[RIGHT] = normalizePlane(glm::vec4(
        matrix[0][3] - matrix[0][0],
        matrix[1][3] - matrix[1][0],
        matrix[2][3] - matrix[2][0],
        matrix[3][3] - matrix[3][0]
    ));

    // Bottom plane
    planes[BOTTOM] = normalizePlane(glm::vec4(
        matrix[0][3] + matrix[0][1],
        matrix[1][3] + matrix[1][1],
        matrix[2][3] + matrix[2][1],
        matrix[3][3] + matrix[3][1]
    ));

    // Top plane
    planes[TOP] = normalizePlane(glm::vec4(
        matrix[0][3] - matrix[0][1],
        matrix[1][3] - matrix[1][1],
        matrix[2][3] - matrix[2][1],
        matrix[3][3] - matrix[3][1]
    ));

    // Near plane
    planes[NEAR] = normalizePlane(glm::vec4(
        matrix[0][3] + matrix[0][2],
        matrix[1][3] + matrix[1][2],
        matrix[2][3] + matrix[2][2],
        matrix[3][3] + matrix[3][2]
    ));

    // Far plane
    planes[FAR] = normalizePlane(glm::vec4(
        matrix[0][3] - matrix[0][2],
        matrix[1][3] - matrix[1][2],
        matrix[2][3] - matrix[2][2],
        matrix[3][3] - matrix[3][2]
    ));
}

glm::vec4 Frustum::normalizePlane(const glm::vec4& plane) const 
{
    float length = glm::length(glm::vec3(plane.x, plane.y, plane.z));
    if (length > 0.0f) {
        return plane / length;
    }
    return plane;
}

float Frustum::getDistanceToPlane(const glm::vec3& point, const glm::vec4& plane) const 
{
    return glm::dot(glm::vec3(plane.x, plane.y, plane.z), point) + plane.w;
}

bool Frustum::isAABBOutsidePlane(const glm::vec3& min, const glm::vec3& max, const glm::vec4& plane) const 
{
    glm::vec3 positiveVertex(
        plane.x > 0.0f ? max.x : min.x,
        plane.y > 0.0f ? max.y : min.y,
        plane.z > 0.0f ? max.z : min.z
    );

    return getDistanceToPlane(positiveVertex, plane) < 0.0f;
}

bool Frustum::isSphereOutsidePlane(const glm::vec3& center, float radius, const glm::vec4& plane) const 
{
    float distance = getDistanceToPlane(center, plane);
    return distance < -radius;
}