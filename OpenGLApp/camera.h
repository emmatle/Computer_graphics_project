#pragma once

#include "utils.h"
#include "object.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera : public Object {
    float _fov;
    float _aspect;

    mutable glm::mat4 projectionMatrix;
    mutable bool projectionDirty = true;
    mutable float lastDistance;
    mutable bool lastMode;

    // Standard math constants in Radians
    static constexpr float MAX_FOV = 120.f;
    static constexpr float MAX_PITCH = 89.f;
    static constexpr float MIN_CLIPPING = 0.01f;
    static constexpr float MAX_CLIPPING = 40.f;

public:
    const float &fov = _fov;
    const float &aspect = _aspect;
    bool costrain;

    Camera(glm::vec3 pos = {}, glm::vec3 rot = {}, float fov = glm::radians(45.f),
           float asp = 1.f, bool costrain = true)
        : Object(pos, rot), _fov(fov), _aspect(asp), costrain(costrain) {
    }

    void setScale(const glm::vec3 &scl) = delete;

    void setFov(float radians) {
        _fov = radians;
        projectionDirty = true;
    }

    void setAspect(float asp) {
        _aspect = asp;
        projectionDirty = true;
    }

    void yaw(float radians) {
        // Use WRLD_UP (0,1,0) and safe=false (Euler Mode) to prevent roll
        rotate(WRLD_UP, radians, false);
    }

    void pitch(float radians) {
        if (costrain) {
            // Use the base class Euler cache (_rotation.x is Pitch)
            float pitch = rotation.x + radians;
            if (pitch > glm::radians(MAX_PITCH) && pitch < glm::radians(180.f)) {
                pitch = glm::radians(MAX_PITCH);
            }
            if (pitch < glm::radians(360.f - MAX_PITCH) && pitch >= glm::radians(180.f)) {
                pitch = glm::radians(360.f - MAX_PITCH);
            }

            // Rotate around WRLD_RIGHT (1,0,0) in Euler Mode
            setRotation({pitch, rotation.y, rotation.z}, false);
        } else {
            rotate(WRLD_RIGHT, radians, false);
        }
    }

    void roll(float radians) {
        rotate(WRLD_FRONT, radians, false);
    }

    void zoom(float radians) {
        _fov += radians;
        if (_fov < glm::radians(1.f)) _fov = glm::radians(1.f);
        if (costrain && fov > glm::radians(MAX_FOV)) _fov = glm::radians(MAX_FOV);
        projectionDirty = true;
    }

    // Object inverse matrix wrapper
    const glm::mat4 &getViewMatrix() const {
        return getInverseModelMatrix();
    }

    const glm::mat4 &getProjectionMatrix(float distance = MAX_CLIPPING, bool persp = true) const {
        if (projectionDirty || distance != lastDistance || persp != lastMode) {
            if (persp)
                projectionMatrix = glm::perspective(_fov, _aspect, MIN_CLIPPING, distance);
            else projectionMatrix = glm::ortho(-_aspect, _aspect, -1.f, 1.f, MIN_CLIPPING, distance);
            lastDistance = distance;
            lastMode = persp;
            projectionDirty = false;
        }
        return projectionMatrix;
    }
};
