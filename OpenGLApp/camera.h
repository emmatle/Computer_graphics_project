#include "object.h"

/**
 * @brief Represents a perspective camera, extending Object with view/projection logic.
 */
class Camera : public Object {
    static constexpr float MAX_FOV = 90.f;
    static constexpr float MAX_PITCH = 89.f; // Prevent flipping (pitch clamp)
    static constexpr float MIN_CLIPPING = 0.01f;

public:
    float fov;
    bool costrain; // Flag to enable pitching limits/constraints

    Camera(glm::vec3 pos = {}, glm::vec3 rot = {}, float fov = 45.f,
           bool costrain = false, float near = 0.1f, float far = 100.f)
            : fov(fov), costrain(costrain) {
        position = pos;
        rotation = rot;
        scale = {1.f, 1.f, 1.f};
        update();
    }

    // Override local rotation functions to use Euler accumulation directly
    void yaw(float degrees) {
        rotation.y += degrees;
        update();
    }

    void pitch(float degrees) {
        rotation.x += degrees;
        update();
    }

    void roll(float degrees) {
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
        return glm::lookAt(position, position + front, up);
    }

    // Returns the Projection Matrix (Perspective or Orthographic)
    glm::mat4 getProjectionMatrix(float asp, float maxDist = 100.f, bool persp = true) const {
        if (persp)
            // Creates a perspective matrix (for 3D rendering)
            return glm::perspective(glm::radians(fov), asp, MIN_CLIPPING,
                                    maxDist);

        // Creates an orthographic matrix (for 2D rendering or special effects)
        return glm::ortho(-asp, asp, -1.f, 1.f, MIN_CLIPPING,
                          maxDist);
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
        up = glm::normalize(
                glm::cross(right, front));     // Recalculate up vector (Gram-Schmidt process for orthogonal axes)

        // Normalize Yaw angle
        if (rotation.y >= 360.f) rotation.y -= 360.f;
        if (rotation.y < 0.f) rotation.y += 360.f;
    }
};