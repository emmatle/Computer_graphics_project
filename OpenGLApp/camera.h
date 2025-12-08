#include "object.h"

/**
 * @brief Represents a perspective camera, extending Object with view/projection logic.
 */
class Camera : public Object {
    constexpr static const float MAX_FOV = 90.0f;
    constexpr static const float MAX_PITCH = 89.0f; // Prevent flipping (pitch clamp)

public:
    float fov;
    float nearPlane;
    float farPlane;
    bool costrain; // Flag to enable pitching limits/constraints

    Camera(glm::vec3 pos = {}, glm::vec3 rot = {}, float fov = 45.f,
           bool costrain = false, float nearPlane = 0.1f, float farPlane = 100.f)
            : fov(fov), nearPlane(nearPlane), farPlane(farPlane), costrain(costrain) {
        position = pos;
        rotation = rot;
        scale = {1.0f, 1.0f, 1.0f};
        update();
    }

    // Horizontal-only movement (keeps the camera on the plane)
    void walk(glm::vec3 dir, float amount) {
        dir.y = 0.f; // Remove vertical component from direction
        position += glm::normalize(dir) * amount;
    }

    // Override local rotation functions to use Euler accumulation directly
    // Cameras often favor simple Euler controls over quaternion safety.
    void yaw(float degrees, bool safe = false) override {
        rotation.y += degrees;
        update();
    }

    void pitch(float degrees, bool safe = false) override {
        rotation.x += degrees;
        update();
    }

    void roll(float degrees, bool safe = false) override {
        rotation.z += degrees;
        update();
    }

    // Adjusts the field of view (zoom)
    void zoom(float amount) {
        fov += amount;
        if (fov < 1.f) fov = 1.f;
        if (costrain && fov > MAX_FOV) fov = MAX_FOV;
    }

    // Returns the View Matrix (inverse transformation of the camera's model matrix)
    glm::mat4 getViewMatrix() const {
        return glm::lookAt(position,        // Eye position
                           position + front, // Target position (where the camera is looking)
                           up);              // Up vector
    }

    // Returns the Projection Matrix (Perspective or Orthographic)
    glm::mat4 getProjectionMatrix(float aspectRatio, float maxDistance = 0.0f, bool perspective = true) const {
        if (perspective)
            // Creates a perspective matrix (for 3D rendering)
            return glm::perspective(glm::radians(fov), aspectRatio, nearPlane,
                                    maxDistance > 0.0f ? maxDistance : farPlane);

        // Creates an orthographic matrix (for 2D rendering or special effects)
        return glm::ortho(-aspectRatio, aspectRatio, -1.f, 1.f, nearPlane,
                          maxDistance > 0.0f ? maxDistance : farPlane);
    }

    // Override update to apply constraints specific to a Camera
    void update() override {
        if (!costrain) return Object::update();

        // Clamp pitch to prevent the camera from flipping over
        if (rotation.x > MAX_PITCH) rotation.x = MAX_PITCH;
        if (rotation.x < -MAX_PITCH) rotation.x = -MAX_PITCH;

        // Calculate the rotation quaternion from the constrained Euler angles
        glm::quat orientation = glm::quat(glm::radians(rotation));

        // Calculate the local axes from the orientation
        front = glm::normalize(orientation * WRLD_FRONT);
        right = glm::normalize(glm::cross(front, WRLD_UP)); // Calculate right vector
        up = glm::normalize(glm::cross(right, front));     // Recalculate up vector (Gram-Schmidt process for orthogonal axes)

        // Normalize Yaw angle
        if (rotation.y >= 360.f) rotation.y -= 360.f;
        if (rotation.y < 0.f) rotation.y += 360.f;
    }
};