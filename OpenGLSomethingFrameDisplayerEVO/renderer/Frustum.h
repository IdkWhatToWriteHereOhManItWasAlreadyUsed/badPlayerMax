#pragma once

#include <glm/glm.hpp>
#include <array>

#undef NEAR
#undef FAR

class Frustum {
public:
    enum Plane {
        LEFT = 0,
        RIGHT,
        BOTTOM, 
        TOP,
        NEAR,
        FAR,
        COUNT
    };

    void update(const glm::mat4& viewProjectionMatrix);
    bool isAABBVisible(const glm::vec3& min, const glm::vec3& max) const;
    bool isSphereVisible(const glm::vec3& center, float radius) const;
    bool isPointVisible(const glm::vec3& point) const;
    glm::vec4 getPlane(Plane plane) const;

private:
    std::array<glm::vec4, COUNT> planes;

    void extractPlanes(const glm::mat4& matrix);
    glm::vec4 normalizePlane(const glm::vec4& plane) const;
    float getDistanceToPlane(const glm::vec3& point, const glm::vec4& plane) const;
    bool isAABBOutsidePlane(const glm::vec3& min, const glm::vec3& max, const glm::vec4& plane) const;
    bool isSphereOutsidePlane(const glm::vec3& center, float radius, const glm::vec4& plane) const;
};