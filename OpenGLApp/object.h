#pragma once

#include "utils.h"

#include <glm/glm.hpp>
#include <nlohmann/json_fwd.hpp>

class Model;
class Shader;

/**
 * @brief Base class for game objects.
 */
class Object {
    // IAssetManager *assetManager;
public:
    // Constant definitions for the world's axis-aligned direction vectors
    constexpr static glm::vec3 WRLD_FRONT{0.f, 0.f, -1.f};
    constexpr static glm::vec3 WRLD_RIGHT{1.f, 0.f, 0.f};
    constexpr static glm::vec3 WRLD_UP{0.f, 1.f, 0.f};

    int id; // Unique identifier (0 for the background)
    std::string name;

    glm::vec3 position;
    glm::vec3 rotation; // Euler angles in degrees (used for UI/serialization)
    glm::vec3 scale;

    // Local coordinate system vectors (derived from rotation)
    glm::vec3 front;
    glm::vec3 right;
    glm::vec3 up;

    Model *model;
    std::string modelPath;
    std::vector<AABB> collisions;

    Object(glm::vec3 pos = {}, glm::vec3 rot = {}, glm::vec3 scl = {1.f, 1.f, 1.f}, std::string path = "",
           int id = 0, std::string name = "");

    Object(nlohmann::json j);

    // Move the object by a normalized direction vector and an amount
    void move(const glm::vec3 &dir, float amount, bool walk = false);

    // Set rotation directly
    void setRotation(const glm::vec3 &rot = {}, bool safe = false);

    // Rotate along an arbitrary axis
    void rotate(const glm::vec3 &axis, float degrees, bool safe = false);

    // Local rotations based on the current UP, RIGHT, and FRONT vectors
    virtual void yaw(float degrees, bool safe = false);

    virtual void pitch(float degrees, bool safe = false);

    virtual void roll(float degrees, bool safe = false);

    // Returns rotation matrix (Euler fallback)
    // NOTE: This uses T-X-Y-Z order rotation, which is prone to gimbal lock
    glm::mat4 getRotationMatrix() const;

    // Generates the final Model Matrix (Translation * Rotation * Scale)
    glm::mat4 getModelMatrix() const;

    // Recalculates the front/right/up vectors from the stored Euler angles
    virtual void update();

    void checkCollision(Object &other);

private:
    static glm::vec3 resolveCollision(glm::vec3 pos, glm::vec3 min, glm::vec3 max);
};
