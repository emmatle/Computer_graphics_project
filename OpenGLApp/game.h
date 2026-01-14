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
        InGame,
        Inspection,
        Credits
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
    std::vector<Animation*> activeAnimations;

    Object *hoveredObj= nullptr;
    Object inventoryObjs[10]; // TODO: Implement inventory.
    Object inspectedObj;
    std::shared_ptr<Object> selectedObj = nullptr;

    // Camera player{{2.24f, 0.93f, 1.17f}, {glm::radians(-5.2f), glm::radians(-50.f), 0.f}}; // Blender render cam
    Camera player{{0.0f, 1.75f, 0.0f}}; // Spawning point
    Camera fixed{{0.0f, 0.0f, 1.f}};
    Camera easleView;
    Camera pictureView;

    static constexpr glm::vec3 LIGHT_COLOR = {1.0f, 0.82f, 0.62f};

    Light lights[3] = {
        {{-2.56f, 1.18f, -0.09f}, LIGHT_COLOR, }, // Window 1
        {{5.0f, 1.6f, -3.0f}, LIGHT_COLOR, },  // Window 2
        {{7.32f, 1.80f, 1.14f}, LIGHT_COLOR, } // Window 3

    };

    Scene room = {{}, &player, {{lights[0], lights[1], lights[2]}}};
    Scene inventory = {
        {&inspectedObj}, &fixed, {
            {{-1.0f, 1.0f, 1.0f}, {1.0f, 0.82f, 0.62f}}, // Key light 4000K
            {{1.0f, -1.0f, -1.0f}, {0.78f, 0.82, 1.0f}} // Back light 8000K
        }
    };
    Scene portals[2] = {
        {{}, {}, {{{-1.0f, 2.0f, -1.0f}}}},
        {
            {}, &easleView, {
                {{-1.0f, 1.0f, 1.0f}, {1.0f, 0.82f, 0.62f}}, // Key light 4000K
                {{1.0f, -1.0f, -2.0f}, {0.776f, 0.824, 1.0f}} // Back light 8000K
            }
        }
    };

    float playerSpeed = 2.0f;
    int playerScore = 0;
    float mouseSensitivity = 0.05f;
    bool mouseDrag = false;

    // Safe code entry
    std::string enteredCode;
    std::string password = "4687391";

    struct Portal {
        std::vector<std::shared_ptr<Object> > objs;
        bool isSolved = false;
    } canvas[2];

    bool loadObjects();

    void update();

    Object *getObject(const std::string &name);

    Object *findObject(int id);

    void viewCanvas(int index); // TODO

    void useObject(Object *obj);

    // --- Callbacks ---

    void onKey();

    void onMouseButton();

    void onMouseMovement(float xdelta, float ydelta);

    void onMouseScroll(float yoffset);

    void onResize(int width, int height);
};
