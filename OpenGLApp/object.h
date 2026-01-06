#pragma once

#include "utils.h"

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <nlohmann/json_fwd.hpp>

class Model;

class Object {
    glm::vec3 _position;
    glm::vec3 _rotation; // Euler angles cache
    glm::vec3 _scale;
    glm::quat orientation; // Used for preventing gimbal lock

    glm::vec3 _front;
    glm::vec3 _right;
    glm::vec3 _up;

    // Model and inverse model matrices cache
    mutable glm::mat4 modelMatrix;
    mutable glm::mat4 inverseModelMatrix;
    mutable bool modelDirty = true;
    mutable bool inverseDirty = true;

    // Updates front/right/up and the rotation Euler cache
    void update(bool safe = true);

public:
    const glm::vec3 &position = _position;
    const glm::vec3 &rotation = _rotation;
    const glm::vec3 &scale = _scale;

    const glm::vec3 &front = _front;
    const glm::vec3 &right = _right;
    const glm::vec3 &up = _up;

    int id;
    std::string name;
    Model *model;
    std::string modelPath;
    std::vector<AABB> collisions;

    // World Constants
    constexpr static glm::vec3 WRLD_FRONT{0.f, 0.f, -1.f};
    constexpr static glm::vec3 WRLD_RIGHT{1.f, 0.f, 0.f};
    constexpr static glm::vec3 WRLD_UP{0.f, 1.f, 0.f};

    Object(const glm::vec3 &pos = {}, const glm::vec3 &rot = {}, const glm::vec3 &scl = {1.f, 1.f, 1.f},
           std::string path = "", int id = 0, std::string name = "");

    Object(nlohmann::json j);

    Object(const Object &other);

    // Copy Assignment Operator
    Object &operator=(const Object &other);

    // Setters
    void setPosition(const glm::vec3 &pos = {});

    void setRotation(const glm::vec3 &radians = {}, bool safe = false);

    void setScale(const glm::vec3 &scl = {1.f, 1.f, 1.f});

    void move(const glm::vec3 &dir, float amount, bool walk = false);

    void rotate(const glm::vec3 &axis, float radians, bool safe = false);

    const glm::mat4 &getModelMatrix() const;

    const glm::mat4 &getInverseModelMatrix() const;

    bool checkCollision(Object &other, bool pushOut = false);
};
