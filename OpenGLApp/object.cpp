#include "object.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <nlohmann/json.hpp>

Object::Object(glm::vec3 pos, glm::vec3 rot, glm::vec3 scl, std::string path, int id, std::string name) : position(pos),
    rotation(rot),
    scale(scl),
    modelPath( std::move(path)),
    id(id),
    name(std::move(name)) {
    update(); // Initialize front/right/up vectors
}

Object::Object(nlohmann::json j) : Object() {
    if (j.contains("id")) id = j["id"]; // ID is optional
    if (j.contains("name")) name = j["name"]; // Name is optional
    if (j.contains("position")) position = {j["position"][0], j["position"][1], j["position"][2]};
    if (j.contains("rotation")) rotation = {j["rotation"][0], j["rotation"][1], j["rotation"][2]};
    if (j.contains("scale")) scale = {j["scale"][0], j["scale"][1], j["scale"][2]};
    if (j.contains("model")) modelPath = j["model"]; // Model is optional
}

// Move the object by a normalized direction vector and an amount
void Object::move(const glm::vec3 &dir, float amount, bool walk) {
    position += glm::normalize(walk ? glm::vec3{dir.x, 0.f, dir.z} : dir) * amount;
}

// Set rotation directly
void Object::setRotation(const glm::vec3 &rot, bool safe) {
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
void Object::rotate(const glm::vec3 &axis, float degrees, bool safe) {
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
void Object::yaw(float degrees, bool safe) {
    rotate(up, degrees, safe); // Rotation around the local up axis
}

void Object::pitch(float degrees, bool safe) {
    rotate(right, degrees, safe); // Rotation around the local right axis
}

void Object::roll(float degrees, bool safe) {
    rotate(front, degrees, safe); // Rotation around the local front axis
}

// Returns rotation matrix (Euler fallback)
// NOTE: This uses T-X-Y-Z order rotation, which is prone to gimbal lock
glm::mat4 Object::getRotationMatrix() const {
    glm::mat4 rot(1.f);
    // Apply YAW (Y-axis), then PITCH (X-axis), then ROLL (Z-axis)
    rot = glm::rotate(rot, glm::radians(rotation.y), glm::vec3(0.f, 1.f, 0.f));
    rot = glm::rotate(rot, glm::radians(rotation.x), glm::vec3(1.f, 0.f, 0.f));
    rot = glm::rotate(rot, glm::radians(rotation.z), glm::vec3(0.f, 0.f, 1.f));
    return rot;
}

// Generates the final Model Matrix (Translation * Rotation * Scale)
glm::mat4 Object::getModelMatrix() const {
    glm::mat4 model(1.f);
    model = glm::translate(model, position);

    // Create a rotation matrix directly from the front/right/up vectors
    // This is safe even if rotation was handled by quaternions.
    glm::mat4 rot(1.f);
    rot[0] = glm::vec4(right, 0.f);
    rot[1] = glm::vec4(up, 0.f);
    rot[2] = glm::vec4(-front, 0.f); // In OpenGL, -Z is the forward view direction

    model *= rot;

    model = glm::scale(model, scale);
    return model;
}

// Recalculates the front/right/up vectors from the stored Euler angles
void Object::update() {
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

void Object::checkCollision(Object &other) {
    // 1. Get the World-to-Local matrix
    glm::mat4 modelMatrix = getModelMatrix();
    glm::mat4 worldToLocal = glm::inverse(modelMatrix);

    // 2. Move player into the box's local space
    glm::vec4 localPos4 = worldToLocal * glm::vec4(other.position, 1.0f);
    glm::vec3 localPos = glm::vec3(localPos4);

    for (auto &AABB: collisions) {
        // 3. Resolve collision in Local Space
        glm::vec3 correctedLocalPos = resolveCollision(localPos, AABB.min, AABB.max);

        // 4. If the position changed, move back to World Space
        if (correctedLocalPos != localPos) {
            glm::vec4 worldPos4 = modelMatrix * glm::vec4(correctedLocalPos, 1.0f);
            other.position = glm::vec3(worldPos4);

            // Re-update localPos for next bound check in this object
            localPos = correctedLocalPos;
        }
    }
}

glm::vec3 Object::resolveCollision(glm::vec3 pos, glm::vec3 min, glm::vec3 max) {
    // Check if player is inside the box (AABB check)
    if (pos.x > min.x && pos.x < max.x &&
        pos.y > min.y && pos.y < max.y && // Added Y check for stability
        pos.z > min.z && pos.z < max.z) {
        // Calculate penetration depth for each face
        float distLeft = pos.x - min.x;
        float distRight = max.x - pos.x;
        float distBottom = pos.y - min.y;
        float distTop = max.y - pos.y;
        float distFront = pos.z - min.z;
        float distBack = max.z - pos.z;

        // Find the smallest distance (the "path of least resistance")
        float minDist = std::min({distLeft, distRight, distBottom, distTop, distFront, distBack});

        // Push out only on the axis of smallest penetration
        if (minDist == distLeft) pos.x = min.x;
        else if (minDist == distRight) pos.x = max.x;
        else if (minDist == distBottom) pos.y = min.y;
        else if (minDist == distTop) pos.y = max.y;
        else if (minDist == distFront) pos.z = min.z;
        else if (minDist == distBack) pos.z = max.z;
    }
    return pos;
}
