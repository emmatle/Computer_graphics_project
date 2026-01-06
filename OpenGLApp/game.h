#pragma once

#include "object.h"
#include "camera.h"

#include <glm/glm.hpp>

/**
 * @brief Manages game logic and handles objects and player state and input.
 */
class Game {
    static constexpr float SPRINT = 2.0f;
    static constexpr float INSPECT_ANGULAR_SPEED = glm::radians(90.f);

public:
    static constexpr glm::vec4 BG_COLOR{0.2, 0.3f, 0.4f, 1.f};
    static constexpr glm::vec4 MENU_COLOR{0.f, 0.f, 0.f, 1.f};

    enum Mode {
        Menu,
        Explore,
        Inspect,
    } mode = Explore;

    struct Input {
        // Mouse buttons
        bool lmb = false;
        bool rmb = false;
        bool mmb = false;

        // W/A/S/D keys
        bool w = false;
        bool s = false;
        bool a = false;
        bool d = false;

        bool q = false;
        bool e = false;

        // Function keys
        bool ctrl = false;
        bool shift = false;
        bool space = false;
        bool tab = false;
        bool esc = false;

        // Numeric keys 0-9
        bool nums[10] = {};
    } input;

    std::vector<std::unique_ptr<Object>> objects;

    Camera *cam = new Camera();
    Camera player{{0.3f, 1.75f, -0.5f}, {glm::radians(-12.0f), glm::radians(64.0f), 0.0f}};
    Camera fixed{{0.0f, 0.0f, 1.5f}};
    Camera *current = nullptr;

    Object inspectedObj;
    Object *selectedObj = nullptr;

    float playerSpeed = 2.5f;
    int playerScore = 0;
    float mouseSensitivity = 0.05f;
    bool mouseDrag = false;

    bool loadObjects();

    bool checkCollision(bool pushOut = false);

    void update();

    Object *getObject(const std::string &name);

    Object *selectObject(int id);

    // --- Callbacks ---

    void onKey();

    void onMouseButton();

    void onMouseMovement(float xdelta, float ydelta);

    void onMouseScroll(float yoffset);

    void onResize(int width, int height);
};
