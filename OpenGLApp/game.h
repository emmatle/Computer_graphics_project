#pragma once

#include "object.h"
#include "camera.h"

#include <glm/glm.hpp>

/**
 * @brief Manages game logic and handles objects and player state and input.
 */
class Game {
    static constexpr float SPRINT = 2.0f;
    static constexpr float INSPECT_ROT_SPEED = 80.f;

public:
    static constexpr glm::vec4 BG_COLOR{0.2, 0.3f, 0.4f, 1.f};
    static constexpr glm::vec4 MENU_COLOR{0.f, 0.f, 0.f, 1.f};

    enum Mode {
        Menu,
        Explore,
        Inspect,
    } mode = Explore;

    struct Input {
        bool w = false;
        bool s = false;
        bool a = false;
        bool d = false;
        bool shift = false;
    } input;

    std::vector<std::unique_ptr<Object>> objects;

    Camera player{{0.3f, 1.75f, -0.5f}, {-12.0f, 64.0f, 0.0f}, 45.0f, true};
    Camera fixed{{0.0f, 0.0f, 3.0f}};
    Camera *current = nullptr;

    Object inspectedObj;
    Object *selectedObj = nullptr;

    float playerSpeed = 2.5f;
    float mouseSensitivity = 0.05f;
    bool mouseDrag = false;

    bool loadObjects();

    void checkCollision();

    void update();

    // --- Callbacks ---

    void onKey(int key, int action, int mods);

    void onMouseButton(int button, int action, int objectId);

    void onMouseMovement(float xdelta, float ydelta);

    void onMouseScroll(float yoffset);
};
