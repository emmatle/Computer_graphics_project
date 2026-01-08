#pragma once

#include "scene.h"

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

    enum State {
        Menu,
        InGame,
        Inventory,
    } state = InGame;

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

    std::vector<std::shared_ptr<Object> > objects;

    Object inventoryObjs[10]; // TODO: Implement inventory.
    Object inspectedObj;
    std::shared_ptr<Object> selectedObj = nullptr;

    Camera player{{0.3f, 1.75f, -0.5f}, {glm::radians(-12.0f), glm::radians(64.0f), 0.0f}};
    Camera fixed{{0.0f, 0.0f, 1.f}};
    Camera mirrorView;
    Camera pictureView;

    Scene room = {{}, &player, {{{-1.0f, 2.0f, -1.0f}}}};
    Scene inventory = {
        {&inspectedObj}, &fixed, {
            {{-1.0f, 1.0f, 1.0f}, {1.0f, 0.835f, 0.671f}}, // Key light 4000K
            {{1.0f, -1.0f, -2.0f}, {0.776f, 0.824, 1.f}} // Back light 8000K
        }
    };
    Scene mirror = {{}, &mirrorView, {{{-1.0f, 2.0f, -1.0f}}}};
    Scene picture = {
        {}, &pictureView, {
            {{-1.0f, 1.0f, 1.0f}, {1.0f, 0.835f, 0.671f}}, // Key light 4000K
            {{1.0f, -1.0f, -2.0f}, {0.776f, 0.824, 1.f}} // Back light 8000K
        }
    };

    float playerSpeed = 2.5f;
    int playerScore = 0;
    float mouseSensitivity = 0.05f;
    bool mouseDrag = false;

    bool loadObjects();

    bool checkCollision(bool pushOut = false);

    void update();

    Object *getObject(const std::string &name);

    Object *selectObject(int id);

    void inspectObject(int id);

    void updateMirror(Object &mirror);

    // --- Callbacks ---

    void onKey();

    void onMouseButton();

    void onMouseMovement(float xdelta, float ydelta);

    void onMouseScroll(float yoffset);

    void onResize(int width, int height);
};
