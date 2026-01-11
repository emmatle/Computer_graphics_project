#pragma once

#include "utils.h"

#include <string>
#include <utility>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <nlohmann/json_fwd.hpp>

#include "scene.h"

class Object;
class Camera;
struct Light;

struct Scene {
    std::vector<Object *> objs;
    Camera *cam = nullptr;
    std::vector<Light> lights;
};

class Shader;

struct Light {
    glm::vec3 position;
    glm::vec3 color;
    float intensity;

    // Attenuation constants for Point Lights
    float constant;
    float linear;
    float quadratic;

    Object *parent;

    Light(const glm::vec3 &pos = {}, const glm::vec3 &color = glm::vec3(1.f), float intensity = 1.0f,
          float constant = 0.75f, float linear = 0.5f, float quadratic = 0.25f);

    void apply(const Shader &shader, int index) const;
};


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

    Object *parent; // TODO: Optimize by keeping track of childs and invalidate their matrices when the parent moves.

    // World Constants
    constexpr static glm::vec3 WRLD_FRONT{0.f, 0.f, -1.f};
    constexpr static glm::vec3 WRLD_RIGHT{1.f, 0.f, 0.f};
    constexpr static glm::vec3 WRLD_UP{0.f, 1.f, 0.f};

    Object(const glm::vec3 &pos = {}, const glm::vec3 &rot = {}, const glm::vec3 &scl = {1.f, 1.f, 1.f},
           std::string path = "", int id = 0, std::string name = "");

    Object(const nlohmann::json &j);

    Object(const Object &other);

    virtual ~Object() = default;

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

    friend void from_json(const nlohmann::json &j, Object &obj);
};

inline void from_json(const nlohmann::json &j, Object &obj) {
    obj.id = j.value("id", obj.id);
    obj.name = j.value("name", obj.name);
    obj._position = j.value("position", obj._position);
    if (j.contains("rotation")) {
        obj._rotation = glm::radians(j.at("rotation").get<glm::vec3>());
        obj.orientation = glm::quat(obj._rotation);
    }
    obj._scale = j.value("scale", obj._scale);
    obj.modelPath = j.value("model", obj.modelPath);

    obj.update();
}

inline void to_json(nlohmann::json &j, const Object &obj) {
    j["id"] = obj.id;
    j["name"] = obj.name;
    j["position"] = obj.position;
    j["rotation"] = glm::degrees(obj.rotation);
    j["scale"] = obj.scale;
    j["model"] = obj.modelPath;
}

class Camera : public Object {
    float _fov;
    float _aspect;

    mutable glm::mat4 projectionMatrix;
    mutable bool projectionDirty = true;
    mutable float lastDistance;
    mutable bool lastMode;

    // Constraints and clipping planes
    static constexpr float MAX_FOV = 120.f;
    static constexpr float MAX_PITCH = 89.f;
    static constexpr float MIN_CLIPPING = 0.01f;
    static constexpr float MAX_CLIPPING = 40.f;

public:
    const float &fov = _fov;
    const float &aspect = _aspect;
    bool costrain;

    Camera(glm::vec3 pos = {}, glm::vec3 rot = {}, float fov = glm::radians(45.f), float asp = 1.f,
           bool costrain = true);

    void setScale(const glm::vec3 &scl) = delete;

    void setFov(float radians);

    void setAspect(float asp);

    void yaw(float radians);

    void pitch(float radians);

    void roll(float radians);

    void zoom(float radians);

    // Object inverse matrix wrapper
    const glm::mat4 &getViewMatrix() const;

    const glm::mat4 &getProjectionMatrix(float distance = MAX_CLIPPING, bool persp = true) const;
};

class AnimatedObject : public Object {
public:
    bool isAnimating = false;
    bool isLooping = false;
    float animSpeed = 1.f;
    float animTime = 0.f; // Time elapsed since animation started
    float animDuration = 0.5f;
    float animProgress = 0.f; // 0.f to 1.f

    // Origin used to calculate translations along an axis (initialized on first update)
    glm::vec3 originPosition = glm::vec3(0.f);
    bool originInitialized = false;

    AnimatedObject(const glm::vec3 &pos = {}, const glm::vec3 &rot = {}, const glm::vec3 &scl = {1.f, 1.f, 1.f},
                   std::string path = "", int id = 0, std::string name = "");

    // Start animating; reverse = true plays towards start, false towards end.
    // This no longer forcefully resets animProgress; it sets animSpeed sign and animTime to match current progress.
    void startAnimating(bool reverse = false);

    void playAnimation(float duration = 1.f, float speed = 1.f);

    void stopAnimating();

    void pauseAnimating();

    void resumeAnimating();

    void animate();

    virtual void onUpdate() {
    }; // To be overridden by derived classes

    // open() acts as toggle: if animating it reverses direction; if stopped it starts toward the other state.
    void open();

    void close();
};

class Door : public AnimatedObject {
public:
    float openAngle = glm::radians(105.f);
    float closeAngle = 0.f;
    float currentAngle = 0.f;

    void onUpdate() override;
};

class Slider : public AnimatedObject {
public:
    float openPos = 1.f;
    float closePos = 0.f;
    float currentPos = 0.f;

    void onUpdate() override;
};

class Drawer : public AnimatedObject {
    float openPos = 0.2f;
    float closePos = 0.f;
    float currentPos = 0.f;

    void onUpdate() override;
};
