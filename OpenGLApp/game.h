#pragma once

#include "scene.h"

#include <glm/glm.hpp>

class Renderer;

/**
 * @brief Manages game logic and handles objects and player state and input.
 */
class Game {
    static constexpr float SPRINT = 2.0f;
    static constexpr float INSPECT_ANGULAR_SPEED = glm::radians(90.f);

    Renderer* renderer = nullptr;

public:
    void setRenderer(Renderer* r) { renderer = r; }
    enum State {
        Splashscreen,
        InGame,
        Inventory,
        Canvas,
        GameOver,
        LeaderboardEntry,
        Credits,
    } state = Splashscreen;

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
        bool enter = false;
        bool backspace = false;

        // Numeric keys 0-9
        bool nums[10] = {};

        // All common letters A-Z (leaderboard)
        bool keys[26] = {};
    } input;

    std::string playerName;

    std::vector<std::shared_ptr<Object> > objects;
    Object *hoveredObj = nullptr;
    std::shared_ptr<Object> selectedObj = nullptr;

    // Camera player{{2.24f, 0.93f, 1.17f}, {glm::radians(-5.2f), glm::radians(-50.f), 0.f}}; // Blender render cam
    Camera player{{0.0f, 1.75f, 0.0f}}; // Spawning point
    Camera inventoryView{{0.0f, 0.0f, 1.0f}};
    Camera canvasView{{0.0f, 0.5f, 12.0f}, {}, glm::radians(45.f), 1.f, false, true};

    static constexpr glm::vec3 KEY_LIGHT = {1.0f, 0.82f, 0.62f}; // 4000K warm light
    static constexpr glm::vec3 BACK_LIGHT = {0.78f, 0.82, 1.0f}; // 8000K cool light

    Light lights[3] = {
        {{-2.56f, 1.18f, -0.09f}, KEY_LIGHT,}, // Window 1
        {{5.0f, 1.6f, -3.0f}, KEY_LIGHT,}, // Window 2
        {{7.32f, 1.80f, 1.14f}, KEY_LIGHT,} // Window 3

    };

    Scene room = {{}, &player, {{lights[0], lights[1], lights[2]}}};

    Scene inventory = {
        {}, &inventoryView, {
            {{-1.0f, 1.0f, 1.0f}, KEY_LIGHT},
            {{1.0f, -1.0f, -1.0f}, BACK_LIGHT}
        }
    };
    Scene canvas = {
        {}, &canvasView, {}, glm::vec3(1.f)
    };

    bool mouseDrag = false;

    std::vector<std::pair<std::string, int>> leaderboard;

    // In game stats
    float time = 0.0f;
    float maxTime = 10 * 60.0f; // 10 minutes
    float remainingTime = maxTime;
    float score = 100.f; // Final score = score + time

    int inventoryIndex = -1; // -1 = no selection / empty inventory
    Object* equippedObj = nullptr;
    float playerSpeed = 2.0f;

    bool doorUnlocked = false;
    bool safeUnlocked = false;
    bool pencilsCorrect = false;

    // Palette minigame
    float collectionTimer = 0.0f;
    int collectedItems = 0;
    bool collectionCompleted = false;
    int collectionCounter = 0;

    enum Pencil {
        Red,
        Orange,
        Yellow,
        Green,
        Azure,
        Blue,
        Purple,
    };

    std::vector<Pencil> pencilOrder = {Red, Green, Blue, Yellow, Orange, Purple, Azure};

    // Safe code entry
    std::string enteredCode;
    // Number of pictures (8) + Colored pencils in order (4687391) + Sum of red, green, blue pencils (20)
    std::string password;

    glm::vec3 lastPos{};
    glm::vec3 lastRot{};

    bool loadObjects();

    void loadLeaderboard();
    void drawLeaderboard();
    void drawCredits(float scroll = 0.f);

    void update();

    void draw();

    Object *getObject(const std::string &name);

    Object *findObject(int id);

    void interact(Object *obj);
    void checkPencils();

    void equip(Object* obj);
    void pick(Object* obj);
    void place(Object* obj, Object* placeholder);

    // --- Callbacks ---

    void onKey();

    void onMouseButton();

    void onMouseMovement(float xdelta, float ydelta);

    void onMouseScroll(float yoffset);

    void onResize(int width, int height);
    void onChar(unsigned int codepoint);
};
