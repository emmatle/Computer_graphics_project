#include "scene.h"
#include "shader.h"
#include "model.h"
#include <glm/gtc/matrix_transform.hpp>
#include <nlohmann/json.hpp>

bool Animation::empty() const { return translation == glm::vec3(0.f) && rotation == glm::vec3(0.f); }

void Animation::play(bool reverse) {
    if (reverse) speed = -std::abs(speed);
    else speed = std::abs(speed);

    time = glm::clamp(progress, 0.f, 1.f) * duration;
    isPlaying = true;
}

void Animation::stop() {
    isPlaying = false;
    time = 0.f;
    progress = 0.f;
}

void Animation::toggle() {
    if (isPlaying) {
        speed = -speed;
        time = glm::clamp(progress, 0.f, 1.f) * duration;
        return;
    }
    if (progress >= 1.f) play(true);
    else if (progress <= 0.f) play(false);
    else play(false);
}

void Animation::update(float dt) {
    if (!isPlaying) return;

    time += dt * speed;
    progress = time / duration;
    if (progress < 0.f) {
        if (isLooping) {
            progress += 1.f;
            time = progress * duration;
        } else {
            progress = 0.f;
            isPlaying = false;
            time = 0.f;
        }
    }
    if (progress >= 1.f) {
        if (isLooping) {
            progress -= 1.f;
            time = progress * duration;
        } else {
            progress = 1.f;
            isPlaying = false;
            time = duration;
        }
    }
}

void Animation::reset() {
    originInitialized = false;
    progress = 0.f;
}

void Animation::apply(Object &obj) {
    if (!originInitialized) {
        originPosition = obj.position;
        originOrientation = obj.orientation;
        originInitialized = true;
    }

    glm::quat newOrientation = originOrientation * glm::quat(rotation * progress);
    glm::vec3 newPosition = originPosition + translation * progress;

    if (pivot != glm::vec3(0.f)) {
        // Calculate the fixed world pivot using initial state
        glm::vec3 worldPivot = originPosition + originOrientation * pivot;
        // Calculate the new position such that the world pivot remains fixed
        newPosition = worldPivot - newOrientation * pivot;
        // If there's also translation in the animation, add it
        newPosition += originOrientation * (translation * progress);
    }

    if (translation != glm::vec3(0.f)) obj.setPosition(newPosition);
    if (rotation != glm::vec3(0.f)) obj.setRotation(newOrientation, pivot);
}

Light::Light(const glm::vec3 &pos, const glm::vec3 &color, float intensity, float constant, float linear,
             float quadratic)
    : position(pos), color(color), intensity(intensity), constant(constant), linear(linear), quadratic(quadratic),
      parent(nullptr) {}

void Light::apply(const Shader &shader, int index) const {
    std::string base = "lights[" + std::to_string(index) + "]";
    glm::vec3 pos = parent ? parent->getModelMatrix() * glm::vec4(position, 1.f) : position;
    shader.set((base + ".position").c_str(), pos);
    shader.set((base + ".color").c_str(), color * intensity);
    shader.set((base + ".constant").c_str(), constant);
    shader.set((base + ".linear").c_str(), linear);
    shader.set((base + ".quadratic").c_str(), quadratic);
}

Object::Object(const glm::vec3 &pos, const glm::vec3 &rot, const glm::vec3 &scl, std::string path, int id,
               std::string name, Object *parent)
    : _position(pos), _rotation(rot), _scale(scl), _orientation(glm::quat(rot)), id(id),
      name(std::move(name)), model(nullptr), modelPath(std::move(path)), parent(parent) {
    update();
}

Object::Object(const nlohmann::json &j) : Object() {
    *this = j.get<Object>();
}

Object::Object(const Object &other)
    : Object(other.position, other.rotation, other.scale, other.modelPath, other.id, other.name, other.parent) {
    model = other.model, animation = other.animation;
}

// Copy assignment operator
Object &Object::operator=(const Object &other) {
    if (this == &other) return *this;

    // Copy data values
    _position = other._position;
    _rotation = other._rotation;
    _scale = other._scale;
    _orientation = other._orientation;
    id = other.id;
    name = other.name;
    modelPath = other.modelPath;
    collisions = other.collisions;
    model = other.model;
    parent = other.parent;
    animation = other.animation;

    // Reset flags so the new copy calculates its own matrices
    modelDirty = true;
    inverseDirty = true;
    update();

    return *this;
}

void Object::update(bool safe) {
    _front = glm::normalize(_orientation * WRLD_FRONT);
    _right = glm::normalize(_orientation * WRLD_RIGHT);
    _up = glm::normalize(_orientation * WRLD_UP);

    if (safe) {
        _rotation = glm::eulerAngles(_orientation);
    }

    constexpr float TWO_PI = glm::two_pi<float>();
    for (int i = 0; i < 3; i++) {
        _rotation[i] = fmod(_rotation[i], TWO_PI);
        if (_rotation[i] < 0) _rotation[i] += TWO_PI;
    }
}

void Object::setPosition(const glm::vec3 &pos) {
    _position = pos;
    modelDirty = true;
    inverseDirty = true;
}

void Object::setRotation(const glm::quat &quat, const glm::vec3 &pivot) {
    if (pivot != glm::vec3(0.f)) {
        glm::vec3 oldWorldPivot = _position + _orientation * pivot;
        _orientation = quat;
        _position = oldWorldPivot - _orientation * pivot;
    } else {
        _orientation = quat;
    }

    update(true);
    modelDirty = true;
    inverseDirty = true;
}

void Object::setRotation(const glm::vec3 &radians, bool safe, const glm::vec3 &pivot) {
    glm::quat nextOrientation = glm::quat(radians);

    if (pivot != glm::vec3(0.f)) {
        glm::vec3 oldWorldPivot = _position + _orientation * pivot;
        _orientation = nextOrientation;
        _position = oldWorldPivot - _orientation * pivot;
    } else {
        _orientation = nextOrientation;
    }

    if (!safe) {
        _rotation = radians;
    }

    update(safe);
    modelDirty = true;
    inverseDirty = true;
}

void Object::setScale(const glm::vec3 &scl) {
    _scale = scl;
    modelDirty = true;
    inverseDirty = true;
}

void Object::move(const glm::vec3 &dir, float amount, bool walk) {
    glm::vec3 moveDir = walk ? glm::vec3{dir.x, 0.f, dir.z} : dir;
    setPosition(_position + glm::normalize(moveDir) * amount);
}

void Object::rotate(const glm::vec3 &axis, float radians, bool safe, const glm::vec3 &offset) {
    // Generate the rotation increment
    glm::quat delta = glm::angleAxis(radians, glm::normalize(axis));

    if (offset != glm::vec3(0.0f)) {
        glm::vec3 oldWorldPivot = _position + _orientation * offset;
        if (safe) {
            _orientation = glm::normalize(delta * _orientation);
        } else {
            _rotation += glm::normalize(axis) * radians;
            _orientation = glm::quat(_rotation);
        }
        _position = oldWorldPivot - _orientation * offset;
    } else {
        if (safe) {
            _orientation = glm::normalize(delta * _orientation);
        } else {
            _rotation += glm::normalize(axis) * radians;
            _orientation = glm::quat(_rotation);
        }
    }

    update(safe);
    modelDirty = true;
    inverseDirty = true;
}

const glm::mat4 &Object::getModelMatrix() const {
    if (modelDirty) {
        modelMatrix = glm::translate(glm::mat4(1.f), _position);

        // Construct rotation matrix from orientation vectors
        glm::mat4 rot(1.f);
        rot[0] = glm::vec4(_right, 0.f);
        rot[1] = glm::vec4(_up, 0.f);
        rot[2] = glm::vec4(-_front, 0.f);

        modelMatrix *= rot;
        modelMatrix = glm::scale(modelMatrix, _scale);
        modelDirty = false;
    }
    if (parent) {
        worldMatrix = parent->getModelMatrix() * modelMatrix;
        return worldMatrix;
    }
    return modelMatrix;
}

const glm::mat4 &Object::getInverseModelMatrix() const {
    if (inverseDirty) {
        if (_scale == glm::vec3(1.f)) {
            inverseModelMatrix = glm::lookAt(_position, _position + _front, _up);
        } else {
            inverseModelMatrix = glm::inverse(getModelMatrix());
        }
        inverseDirty = false;
    }
    return inverseModelMatrix;
}

bool Object::checkCollision(Object &other, bool pushOut) {
    if (collisions.empty()) return false;

    bool collide = false;
    // Transform from world space to local
    glm::vec3 localPos = glm::vec3(getInverseModelMatrix() * glm::vec4(other.position, 1.0f));

    for (const auto &box: collisions) {
        // Resolve collision in local space
        if (localPos.x > box.min.x && localPos.x < box.max.x &&
            localPos.y > box.min.y && localPos.y < box.max.y &&
            localPos.z > box.min.z && localPos.z < box.max.z) {
            if (!pushOut) return true;
            collide = true;

            // Simple AABB response (push out on shortest axis)
            float d[] = {
                localPos.x - box.min.x, box.max.x - localPos.x,
                localPos.y - box.min.y, box.max.y - localPos.y,
                localPos.z - box.min.z, box.max.z - localPos.z
            };
            float minD = *std::min_element(std::begin(d), std::end(d));

            if (minD == d[0]) localPos.x = box.min.x;
            else if (minD == d[1]) localPos.x = box.max.x;
            else if (minD == d[2]) localPos.y = box.min.y;
            else if (minD == d[3]) localPos.y = box.max.y;
            else if (minD == d[4]) localPos.z = box.min.z;
            else if (minD == d[5]) localPos.z = box.max.z;

            // Transform back to world space
            other.setPosition(glm::vec3(getModelMatrix() * glm::vec4(localPos, 1.0f)));
        }
    }
    return collide;
}

void Object::onUpdate() {
    if (!animation.empty() && animation.isPlaying) {
        animation.update();
        animation.apply(*this);
    }
}

Camera::Camera(glm::vec3 pos, glm::vec3 rot, float fov, float asp, bool costrain, bool ortho)
    : Object(pos, rot), _fov(fov), _aspect(asp), costrain(costrain), ortho(ortho) {}

void Camera::setFov(float radians) {
    _fov = radians;
    projectionDirty = true;
}

void Camera::setAspect(float asp) {
    _aspect = asp;
    projectionDirty = true;
}

void Camera::yaw(float radians) {
    // Use WRLD_UP (0,1,0) and safe = false (Euler Mode) to prevent roll
    rotate(WRLD_UP, radians, false);
}

void Camera::pitch(float radians) {
    if (costrain) {
        // Use the base class Euler cache (_rotation.x is pitch)
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

void Camera::roll(float radians) {
    rotate(WRLD_FRONT, radians, false);
}

void Camera::zoom(float radians) {
    _fov += radians;
    if (_fov < glm::radians(1.f)) _fov = glm::radians(1.f);
    if (costrain && fov > glm::radians(MAX_FOV)) _fov = glm::radians(MAX_FOV);
    projectionDirty = true;
}

// Object inverse matrix wrapper
const glm::mat4 &Camera::getViewMatrix() const {
    return getInverseModelMatrix();
}

const glm::mat4 &Camera::getProjectionMatrix(float distance) const {
    if (projectionDirty || distance != lastDistance) {
        if (!ortho)
            projectionMatrix = glm::perspective(_fov, _aspect, MIN_CLIPPING, distance);
        else projectionMatrix = glm::ortho(-ORTHOGRAPHIC_SCALE * _aspect, ORTHOGRAPHIC_SCALE * _aspect,
                                           -ORTHOGRAPHIC_SCALE, ORTHOGRAPHIC_SCALE, MIN_CLIPPING, distance);
        lastDistance = distance;
        projectionDirty = false;
    }
    return projectionMatrix;
}
