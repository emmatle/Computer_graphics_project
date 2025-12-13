#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#define GLM_ENABLE_EXPERIMENTAL // Required for some extended GLM functionality

#include <glm/gtx/euler_angles.hpp> // Utilities for converting between Euler angles and other formats

#include "texture.h"

/**
 * @brief Base class for any rotatable, translatable, and scalable object in the 3D world.
 */
class Object {
public:
    // Constant definitions for the world's axis-aligned direction vectors.
    constexpr static const glm::vec3 WRLD_FRONT{0.0f, 0.0f, -1.0f};
    constexpr static const glm::vec3 WRLD_RIGHT{1.0f, 0.0f, 0.0f};
    constexpr static const glm::vec3 WRLD_UP{0.0f, 1.0f, 0.0f};

    int id = 0; // Unique identifier
    std::string name;

    glm::vec3 position;
    glm::vec3 rotation;     // Euler angles in degrees (used for UI/serialization)
    glm::vec3 scale;

    // Local coordinate system vectors (derived from rotation)
    glm::vec3 front;
    glm::vec3 right;
    glm::vec3 up;

    std::string texture;
    Texture tex;

    Object(glm::vec3 pos = {}, glm::vec3 rot = {}, glm::vec3 scl = {1, 1, 1})
            : position(pos), rotation(rot), scale(scl) {
        static int nObjects = 0;
//        id = ++nObjects; // Assign unique ID
        update();        // Initialize front/right/up vectors
    }

    Object(std::string name, glm::vec3 pos = {}, glm::vec3 rot = {}, glm::vec3 scl = {1, 1, 1})
            : name(std::move(name)), position(pos), rotation(rot), scale(scl) {
        static int nObjects = 0;
        id = ++nObjects; // Assign unique ID
        update();        // Initialize front/right/up vectors
    }

    // Move the object by a normalized direction vector and an amount
    void move(const glm::vec3 &dir, float amount, bool walk = false) {
        position += glm::normalize(walk ? glm::vec3{dir.x, 0.f, dir.z} : dir) * amount;
    }

    // Set rotation directly
    void setRotation(const glm::vec3 &rot = {}, bool safe = false) {
        if (!safe) {
            rotation = rot;
            update(); // Recompute local axes using Euler angles
            return;
        }

        // Use Quaternion for safe rotation (avoids gimbal lock)
        glm::quat q = glm::quat(glm::radians(rot));

        // Calculate the local axes using quaternion rotation
        front = glm::normalize(q * WRLD_FRONT);
        right = glm::normalize(q * WRLD_RIGHT);
        up = glm::normalize(q * WRLD_UP);

        // Convert the quaternion back to Euler angles for storage/UI
        rotation = glm::degrees(glm::eulerAngles(q));
    }

    // Rotate along an arbitrary axis
    void rotate(const glm::vec3 &axis, float degrees, bool safe = false) {
        if (!safe) {
            rotation += glm::normalize(axis) * degrees;
            update(); // Simple Euler angle accumulation (risks gimbal lock)
            return;
        }

        // Use Quaternions for safe accumulation
        glm::quat q = glm::quat(glm::radians(rotation));
        glm::quat dq = glm::angleAxis(glm::radians(degrees), glm::normalize(axis));
        q = glm::normalize(dq * q); // New rotation is (delta rotation * current rotation)

        // Recalculate local axes
        front = glm::normalize(q * WRLD_FRONT);
        right = glm::normalize(q * WRLD_RIGHT);
        up = glm::normalize(q * WRLD_UP);

        // Update stored Euler angles
        rotation = glm::degrees(glm::eulerAngles(q));
    }

    // Local rotations based on the current UP, RIGHT, and FRONT vectors
    virtual void yaw(float degrees, bool safe = false) {
        rotate(up, degrees, safe); // Rotation around the local up axis
    }

    virtual void pitch(float degrees, bool safe = false) {
        rotate(right, degrees, safe); // Rotation around the local right axis
    }

    virtual void roll(float degrees, bool safe = false) {
        rotate(front, degrees, safe); // Rotation around the local front axis
    }

    // Returns rotation matrix (Euler fallback)
    // NOTE: This uses T-X-Y-Z order rotation, which is prone to gimbal lock
    glm::mat4 getRotationMatrix() const {
        glm::mat4 rot(1.0f);
        // Apply YAW (Y-axis), then PITCH (X-axis), then ROLL (Z-axis)
        rot = glm::rotate(rot, glm::radians(rotation.y), glm::vec3(0, 1, 0));
        rot = glm::rotate(rot, glm::radians(rotation.x), glm::vec3(1, 0, 0));
        rot = glm::rotate(rot, glm::radians(rotation.z), glm::vec3(0, 0, 1));
        return rot;
    }

    // Generates the final Model Matrix (Translation * Rotation * Scale)
    glm::mat4 getModelMatrix() const {
        glm::mat4 model(1.0f);
        model = glm::translate(model, position);

        // Create a rotation matrix directly from the front/right/up vectors
        // This is safe even if rotation was handled by quaternions.
        glm::mat4 rot(1.0f);
        rot[0] = glm::vec4(right, 0.0f);
        rot[1] = glm::vec4(up, 0.0f);
        rot[2] = glm::vec4(-front, 0.0f); // In OpenGL, -Z is the forward view direction

        model *= rot;

        model = glm::scale(model, scale);
        return model;
    }

    // Recalculates the front/right/up vectors from the stored Euler angles
    virtual void update() {
        // Compute axes from Euler angles using a Quaternion intermediate
        glm::quat q = glm::quat(glm::radians(rotation));

        front = glm::normalize(q * WRLD_FRONT);
        right = glm::normalize(q * WRLD_RIGHT);
        up = glm::normalize(q * WRLD_UP);

        // Normalize angles to the range [0, 360)
        for (int i = 0; i < 3; i++) {
            while (rotation[i] >= 360.f) rotation[i] -= 360.f;
            while (rotation[i] < 0.f) rotation[i] += 360.f;
        }
    }
};