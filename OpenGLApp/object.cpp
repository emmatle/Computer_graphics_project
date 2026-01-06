#include "object.h"
#include <glm/gtc/matrix_transform.hpp>
#include <nlohmann/json.hpp>

Object::Object(const glm::vec3 &pos, const glm::vec3 &rot, const glm::vec3 &scl, std::string path, int id,
               std::string name)
    : _position(pos), _rotation(rot), _scale(scl), orientation(glm::quat(rot)), id(id),
      name(std::move(name)), model(nullptr), modelPath(std::move(path)) {
    update();
}

Object::Object(nlohmann::json j) : Object() {
    if (j.contains("id")) id = j["id"];
    if (j.contains("name")) name = j["name"];
    if (j.contains("position")) _position = {j["position"][0], j["position"][1], j["position"][2]};
    if (j.contains("rotation")) {
        glm::vec3 degrees = {j["rotation"][0], j["rotation"][1], j["rotation"][2]};
        _rotation = glm::radians(degrees);
        orientation = glm::quat(_rotation);
    }
    if (j.contains("scale")) _scale = {j["scale"][0], j["scale"][1], j["scale"][2]};
    if (j.contains("model")) modelPath = j["model"];

    update();
}

Object::Object(const Object &other)
    : Object(other.position, other.rotation, other.scale, other.modelPath, other.id, other.name) {
    model = other.model;
}

// Copy assignment operator
Object &Object::operator=(const Object &other) {
    if (this == &other) return *this;

    // Copy data values
    _position = other._position;
    _rotation = other._rotation;
    _scale = other._scale;
    orientation = other.orientation;
    id = other.id;
    name = other.name + "_copy";
    modelPath = other.modelPath;
    collisions = other.collisions;
    model = other.model;

    // Reset flags so the new copy calculates its own matrices
    modelDirty = true;
    inverseDirty = true;
    update();

    return *this;
}

void Object::update(bool safe) {
    _front = glm::normalize(orientation * WRLD_FRONT);
    _right = glm::normalize(orientation * WRLD_RIGHT);
    _up = glm::normalize(orientation * WRLD_UP);

    if (safe) {
        _rotation = glm::eulerAngles(orientation);
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

void Object::setRotation(const glm::vec3 &radians, bool safe) {
    if (safe) {
        // Build from quaternion, then sync _rotation (update)
        orientation = glm::quat(radians);
    } else {
        // Euler fallback
        _rotation = radians;
        orientation = glm::quat(_rotation);
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

void Object::rotate(const glm::vec3 &axis, float radians, bool safe) {
    if (safe) {
        // Quaternion multiplication (avoids gimbal lock), then sync _rotation (update)
        glm::quat delta = glm::angleAxis(radians, glm::normalize(axis));
        orientation = glm::normalize(delta * orientation);
    } else {
        // Euler fallback
        _rotation += glm::normalize(axis) * radians;
        orientation = glm::quat(_rotation);
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
