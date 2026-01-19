#include "application.h"
#include "game.h"
#include "renderer.h"
#include "utils.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <SFML/Audio.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <imgui.h>
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_glfw.h>
#include <nlohmann/json.hpp>

#include "audio_manager.h"
sf::Music Application::backgroundMusicNormal;
sf::Music Application::backgroundMusicLowpass;
bool Application::backgroundLowpassActive = false;
float Application::backgroundMusicPrevVolume = 30.f;

Game Application::game;
Renderer Application::renderer;

GLFWwindow *Application::window = nullptr;
std::string Application::title = "Escape Room";
int Application::width = 1024;
int Application::height = 768;

bool Application::fullscreen = false; // Overrides width and height if true
bool Application::vsync = false;
float Application::mouseSensitivity = 0.1f;
float Application::fontSize = 16.f;
float Application::fps = 0.f; // Average framerate (on 0.5 seconds)

float Application::mouseX = static_cast<float>(width) / 2.f;
float Application::mouseY = static_cast<float>(height) / 2.f;
bool Application::gameFocus = true;
bool Application::firstMouse = true;

sf::Music backgroundMusic(getResourcePath("sounds/BackgroundMusic.mp3"));


void Application::updateTime() {
    static float currentFrame = 0.f;
    static float lastFrame = 0.f;
    static float average = 0.f;
    static float elapsedTime = 0.f;
    static int frameCount = 0;
    static bool initialized = false;

    if (!initialized) {
        glfwSetTime(0.0);
        initialized = true;
    }

    currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    frameCount++;
    elapsedTime += deltaTime;

    if (elapsedTime > 0.5f) {
        average = static_cast<float>(frameCount) / elapsedTime;
        elapsedTime = 0.f;
        frameCount = 0;
    }
    fps = average;
}

// Initializes GLFW, GLAD, and ImGui
void Application::init() {
    loadSettings();

    if (!backgroundMusicNormal.openFromFile(getResourcePath("sounds/BackgroundMusic.mp3"))) {
        std::cerr << "ERROR: can't load " << getResourcePath("sounds/BackgroundMusic.mp3") << std::endl;
    } else {
        backgroundMusicNormal.setVolume(30.f);
        backgroundMusicNormal.setLooping(true);
        backgroundMusicNormal.play();
        backgroundLowpassActive = false;
        backgroundMusicPrevVolume = 30.f;
    }

    if (!backgroundMusicLowpass.openFromFile(getResourcePath("sounds/BackgroundMusicLowPass.mp3"))) {
        std::cerr << "WARNING: can't load " << getResourcePath("sounds/BackgroundMusicLowPass.mp3") << std::endl;
    } else {
        backgroundMusicLowpass.setVolume(30.f);
        backgroundMusicLowpass.setLooping(true);
    }

    AudioManager::instance().init();
    sf::SoundBuffer opendoorBuffer;
    if (!opendoorBuffer.loadFromFile(getResourcePath("sounds/door_opening.mp3"))) {
        std::cerr << "ERROR: can't load " << getResourcePath("sounds/door_opening.mp3") << std::endl;
    } else {
        AudioManager::instance().loadSound("OpenDoor", opendoorBuffer);
    }

    sf::SoundBuffer itempickupBuffer;
    if (!itempickupBuffer.loadFromFile(getResourcePath("sounds/item_pickup.mp3"))) {
        std::cerr << "ERROR: can't load " << getResourcePath("sounds/item_pickup.mp3") << std::endl;
    } else {
        AudioManager::instance().loadSound("CollectItem", itempickupBuffer);
    }

    if (!glfwInit()) {
        std::cerr << "ERROR: failed to initialize GLFW" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Request OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // macOS requirement
#endif

    // Create main window
    if (fullscreen) {
        GLFWmonitor *monitor = glfwGetPrimaryMonitor();
        width = glfwGetVideoMode(monitor)->width;
        height = glfwGetVideoMode(monitor)->height;
        window = glfwCreateWindow(width, height, title.c_str(), monitor, nullptr);
    } else {
        window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    }
    if (!window) {
        std::cerr << "ERROR: failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);

    // Load OpenGL functions
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "ERROR: failed to initialize GLAD" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Updates the Framebuffer size and the aspect ratio
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    aspect = static_cast<float>(width) / static_cast<float>(height);
    fbScale = static_cast<float>(fbWidth) / static_cast<float>(width);
    framebufferSizeCallback(window, fbWidth, fbHeight);

    // Enable VSync
    glfwSwapInterval(vsync);

    // Register callbacks
    glfwSetWindowCloseCallback(window, windowCloseCallback);
    glfwSetWindowSizeCallback(window, windowSizeCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);

    // ImGui Setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplOpenGL3_Init();
    ImGui_ImplGlfw_InitForOpenGL(window, true);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Locks mouse
    ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;

    // Set custom font
    std::string fontPath = getResourcePath("fonts/OpenSans.ttf");
    const char *font = fontPath.c_str();
    ImGui::GetIO().Fonts->AddFontFromFileTTF(font, fontSize);
}

void Application::setBackgroundLowpass(bool enable) {
    if (enable == backgroundLowpassActive) return;
    backgroundLowpassActive = enable;

    if (enable) {
        // salva volume corrente e abbassa la musica di sottofondo
        backgroundMusicPrevVolume = backgroundMusicNormal.getVolume();
        float lowered = std::max(5.f, backgroundMusicPrevVolume * 0.25f); // 25% o minimo 5
        backgroundMusicNormal.setVolume(lowered);
    } else {
        // ripristina il volume salvato
        backgroundMusicNormal.setVolume(backgroundMusicPrevVolume);
    }
}

/* TODO: abstract rendering and game logic from application class:
 *  - all OpenGL gl*() functions should be wrapped inside the Renderer class
 *  - game state should be modified by the Game class methods and members are accessed only if needed
 */
void Application::run() {
    init();

    if (!renderer.compileShaders()) exit(EXIT_FAILURE);

    if (!renderer.loadFont("fonts/Antonio-Bold.ttf")) exit(EXIT_FAILURE);

    // TODO: Add a splashscreen.
    glfwPollEvents();
    Renderer::clear();
    renderer.drawText("Loading...", 25.0f, static_cast<float>(fbHeight) - 25.0f, fbScale,
                      glm::vec3(1.0f, 1.0f, 0.0f));
    glfwSwapBuffers(window);

    if (!game.loadObjects()) exit(EXIT_FAILURE);

    game.setRenderer(&renderer);

    // TODO: free up allocated resources before exiting.
    if (!renderer.genBuffers()) exit(EXIT_FAILURE);

    if (!renderer.loadModels(game.objects)) exit(EXIT_FAILURE);

    // Main Loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        updateTime();

        game.update();

        // Begin new ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (debug) drawDebugMenu();

        game.draw(fbWidth, fbHeight, fbScale);

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    renderer.free();

    terminate();
}

// Shuts down ImGui and GLFW
void Application::terminate() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    storeSettings();
}

// TODO: implement settings and change path.
void Application::loadSettings() {
    std::filesystem::path file = getResourcePath("data.json", true);
    return;

    using json = nlohmann::json;
    std::ifstream in(file);
    if (!in) {
        std::cout << "Settings file " << file << " not found, using defaults" << std::endl; // Not an error
        return;
    }

    in.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    json j;

    try {
        in >> j;
    } catch (std::exception &e) {
        std::cerr << "ERROR: failed to read settings from " << file << ": " << e.what() << std::endl;
        return;
    }

    if (!j.contains("settings")) return;
    const json &s = j["settings"];

    if (s.contains("width")) width = s["width"];
    if (s.contains("height")) height = s["height"];
    if (s.contains("fullscreen")) fullscreen = s["fullscreen"];
    if (s.contains("vsync")) vsync = s["vsync"];
    if (s.contains("mouseSensitivity")) mouseSensitivity = s["mouseSensitivity"];
    if (s.contains("fontSize")) fontSize = s["fontSize"];
}

// TODO: implement settings and change path.
void Application::storeSettings() {
    std::filesystem::path file = getResourcePath("data.json", true);
    return;
    using ojson = nlohmann::ordered_json;

    std::ofstream out(file);
    if (!out) {
        std::cerr << "ERROR: cannot open file " << file << " for writing" << std::endl;
        return;
    }

    ojson j;
    j["settings"] = ojson{
        {"width", width},
        {"height", height},
        {"fullscreen", fullscreen},
        {"vsync", vsync},
        {"mouseSensitivity", mouseSensitivity},
        {"fontSize", fontSize},
    };

    try {
        out.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        out << j.dump(4);
    } catch (std::exception &e) {
        std::cerr << "ERROR: failed to write settings to " << file << ": " << e.what() << std::endl;
    }
}

// Debug UI with camera and object controls
void Application::drawDebugMenu() {
    Camera &cam = game.state == Game::Inventory ? game.inventoryView : game.player;
    Object *obj = nullptr;
    if (game.state == Game::Inventory && !game.inventory.objs.empty()) {
        obj = game.inventory.objs[game.inventoryIndex];
    }

    if (game.state == Game::InGame && game.selectedObj) obj = game.selectedObj.get();

    ImGui::SetNextWindowPos(ImVec2(40.0f, 40.0f), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(240.0f, 600.0f), ImGuiCond_Once);

    ImGui::Begin("Debug Menu");

    ImGui::Text("Time: %.2f    FPS: %.2f", game.time, fps);

    ImGui::SeparatorText("Controls");
    ImGui::BulletText("F1: Toggle Window Focus");
    ImGui::BulletText("F2: Toggle Background Music");
    ImGui::BulletText("F3: Toggle Debug Menu");
    ImGui::BulletText("F4: Quit Application");

    ImGui::BulletText("W/A/S/D: Move/Inspect");
    ImGui::BulletText("Mouse Movement: Look Around");
    ImGui::BulletText("Left Mouse Button: Interact");
    ImGui::BulletText("Mouse Wheel: Change Zoom");
    ImGui::BulletText("Left Shift: Sprint");
    // ImGui::BulletText("0-9: Equip Slot 0-9"); // TODO
    // ImGui::BulletText("Tab: Open Inventory"); // TODO
    // ImGui::BulletText("Esc: Pause The Game"); // TODO

    ImGui::SeparatorText("Camera");
    glm::vec3 camPos = cam.position;
    if (ImGui::SliderFloat3("Position", &camPos[0], -5.f, 5.f)) {
        cam.setPosition(camPos);
    }

    glm::vec3 camDegrees = glm::degrees(cam.rotation);
    if (ImGui::SliderFloat3("Rotation", &camDegrees[0], 0.f, 360.f)) {
        game.player.setRotation(glm::radians(camDegrees)); // Updates the camera
    }

    ImGui::Checkbox("Constrain", &cam.costrain);

    float fovDegrees = glm::degrees(cam.fov);
    if (ImGui::SliderFloat("FOV", &fovDegrees, 1.f, 120.f)) {
        cam.setFov(glm::radians(fovDegrees));
    }

    ImGui::SliderFloat("Speed", &game.playerSpeed, 0.1f, 5.f);

    ImGui::SeparatorText("Game Mode");

    const char *modes[] = {"Menu", "Explore", "Inspect"};
    int modeIndex = game.state;
    if (ImGui::Combo("##Game Mode", &modeIndex, modes, 3)) {
        game.state = static_cast<Game::State>(modeIndex);
    }

    ImGui::SeparatorText("Game Progress");
    ImGui::Text("Brushes Collected: %d/%d", game.collectedItems, 4);
    if (game.collectionTimer > 0) ImGui::Text("Timer: %.2fs", game.collectionTimer);
    ImGui::Text("Entered: %s", game.enteredCode.c_str());
    ImGui::Text("Password: %s", game.password.c_str());

    if (obj) {
        ImGui::SeparatorText(obj->name.c_str());
        glm::vec3 objPos = obj->position;
        if (ImGui::SliderFloat3("Pos##Object Pos", &objPos[0], -5.f, 5.f)) {
            obj->setPosition(objPos);
        }

        glm::vec3 objDegrees = glm::degrees(obj->rotation);
        if (ImGui::SliderFloat3("Rot##Object Rot", &objDegrees[0], 0.f, 360.f)) {
            obj->setRotation(glm::radians(objDegrees)); // Updates the object
        }

        glm::vec3 objScale = obj->scale;
        if (ImGui::SliderFloat3("Scale##Object Scale", &objScale[0], 0.1f, 5.f)) {
            obj->setScale(objScale);
        }
    }

    ImGui::End();
}

// Callback for closing the window
void Application::windowCloseCallback(GLFWwindow *window) {
    glfwSetWindowShouldClose(window, true);
}

// Callback for window resizing
void Application::windowSizeCallback(GLFWwindow *window, int width, int height) {
    Application::width = width;
    Application::height = height;
    aspect = static_cast<float>(width) / static_cast<float>(height);
}

// Callback for framebuffer resizing
void Application::framebufferSizeCallback(GLFWwindow *, int width, int height) {
    glViewport(0, 0, width, height);
    fbWidth = width;
    fbHeight = height;
    renderer.onResize(width, height);
    game.onResize(width, height);
}

// Callback for keyboard key presses
void Application::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    // TODO: Implement a menu for loading, saving and closing the game.
    // Toggle UI focus mode (cursor visibility)
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        if (gameFocus) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // Show cursor
            ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;

            gameFocus = false;
            game.input = {};
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Lock cursor
            glfwSetCursorPos(window, static_cast<float>(fbWidth) / 2.f, static_cast<float>(fbHeight) / 2.f);
            ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;

            gameFocus = true;
            firstMouse = true;
        }
    }
    if (key == GLFW_KEY_F2 && action == GLFW_PRESS) {
        auto &activeMusic = backgroundLowpassActive ? backgroundMusicLowpass : backgroundMusicNormal;
        auto status = activeMusic.getStatus();
        if (status == sf::Sound::Status::Playing) {
            activeMusic.pause();
        } else if (status == sf::Sound::Status::Paused) {
            activeMusic.play();
        }
    }
    if (key == GLFW_KEY_F3 && action == GLFW_PRESS) {
        debug = !debug;
    }
    if (key == GLFW_KEY_F4 && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (!gameFocus) return;

    if (key == GLFW_KEY_W) {
        if (action == GLFW_PRESS) game.input.w = true;
        if (action == GLFW_RELEASE) game.input.w = false;
    }
    if (key == GLFW_KEY_S) {
        if (action == GLFW_PRESS) game.input.s = true;
        if (action == GLFW_RELEASE) game.input.s = false;
    }
    if (key == GLFW_KEY_A) {
        if (action == GLFW_PRESS) game.input.a = true;
        if (action == GLFW_RELEASE) game.input.a = false;
    }
    if (key == GLFW_KEY_D) {
        if (action == GLFW_PRESS) game.input.d = true;
        if (action == GLFW_RELEASE) game.input.d = false;
    }

    if (key == GLFW_KEY_Q) {
        if (action == GLFW_PRESS) game.input.q = true;
        if (action == GLFW_RELEASE) game.input.q = false;
    }
    if (key == GLFW_KEY_E) {
        if (action == GLFW_PRESS) game.input.e = true;
        if (action == GLFW_RELEASE) game.input.e = false;
    }

    if (key == GLFW_KEY_LEFT_CONTROL) {
        if (action == GLFW_PRESS) game.input.ctrl = true;
        if (action == GLFW_RELEASE) game.input.ctrl = false;
    }
    if (key == GLFW_KEY_LEFT_SHIFT) {
        if (action == GLFW_PRESS) game.input.shift = true;
        if (action == GLFW_RELEASE) game.input.shift = false;
    }
    if (key == GLFW_KEY_SPACE) {
        if (action == GLFW_PRESS) game.input.space = true;
        if (action == GLFW_RELEASE) game.input.space = false;
    }
    if (key == GLFW_KEY_TAB) {
        if (action == GLFW_PRESS) game.input.tab = true;
        if (action == GLFW_RELEASE) game.input.tab = false;
    }
    if (key == GLFW_KEY_ESCAPE) {
        if (action == GLFW_PRESS) game.input.esc = true;
        if (action == GLFW_RELEASE) game.input.esc = false;
    }
    if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
        int index = key - GLFW_KEY_1;
        if (action == GLFW_PRESS) game.input.nums[index] = true;
        if (action == GLFW_RELEASE) game.input.nums[index] = false;
    }

    game.onKey();
}

void Application::mouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
    if (!gameFocus) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) game.input.lmb = true;
        if (action == GLFW_RELEASE) game.input.lmb = false;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) game.input.rmb = true;
        if (action == GLFW_RELEASE) game.input.rmb = false;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (action == GLFW_PRESS) game.input.mmb = true;
        if (action == GLFW_RELEASE) game.input.mmb = false;
    }

    game.onMouseButton();
}

// Callback for mouse movement
void Application::cursorPosCallback(GLFWwindow *, double xpos, double ypos) {
    if (firstMouse) {
        mouseX = (float) xpos;
        mouseY = (float) ypos;
        firstMouse = false;
    }

    float xDelta = mouseX - (float) xpos;
    float yDelta = mouseY - (float) ypos;

    mouseX = (float) xpos;
    mouseY = (float) ypos;

    if (gameFocus) game.onMouseMovement(xDelta, yDelta);
}

// Callback for mouse scroll
void Application::scrollCallback(GLFWwindow *window, double xoffset, double yoffset) {
    if (gameFocus) game.onMouseScroll(static_cast<float>(yoffset));
}

int main() {
    Application app;
    app.run();
    return 0;
}
