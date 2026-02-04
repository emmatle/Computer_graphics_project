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
sf::Music Application::backgroundMusic;
sf::Music Application::backgroundMusicLowpass;
bool Application::backgroundLowpassActive = false;

Game Application::game;
Renderer Application::renderer;

GLFWwindow *Application::window = nullptr;
std::string Application::title = "Escape Room";
int Application::width = 1024;
int Application::height = 768;

bool Application::fullscreen = true; // Overrides width and height if true
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
    if (!backgroundMusic.openFromFile(getResourcePath("sounds/BackgroundMusic.mp3"))) {
        std::cerr << "ERROR: can't load " << getResourcePath("sounds/BackgroundMusic.mp3") << std::endl;
    } else {
        backgroundMusic.setVolume(30.f);
        backgroundMusic.setPitch(8 / 12.f); // -4 semitones
        backgroundMusic.setLooping(true);
        backgroundMusic.play();
        backgroundLowpassActive = false;
    }

    if (!backgroundMusicLowpass.openFromFile(getResourcePath("sounds/BackgroundMusicLowPass.mp3"))) {
        std::cerr << "WARNING: can't load " << getResourcePath("sounds/BackgroundMusicLowPass.mp3") << std::endl;
    } else {
        backgroundMusicLowpass.setVolume(15.f);
        backgroundMusicLowpass.setPitch(8 / 12.f); // -4 semitones
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
    glfwSetCharCallback(window, charCallback);

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

void Application::run() {
    init();

    game.setRenderer(&renderer);

    if (!renderer.compileShaders()) exit(EXIT_FAILURE);

    if (!renderer.loadFont("fonts/Antonio-Bold.ttf")) exit(EXIT_FAILURE);

    glfwPollEvents();
    Renderer::clear();
    renderer.drawText("Loading...", static_cast<float>(fbWidth) * 0.05f, static_cast<float>(fbHeight) * 0.95f, 1.0f,
                      glm::vec3(0.8f));
    glfwSwapBuffers(window);

    if (!game.loadObjects()) exit(EXIT_FAILURE);

    game.loadLeaderboard();

    // TODO: free up allocated resources before exiting
    if (!renderer.genBuffers()) exit(EXIT_FAILURE);

    if (!renderer.loadModels(game.objects)) exit(EXIT_FAILURE);

    // Main Loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        updateTime();

        game.update();

        if (game.close) glfwSetWindowShouldClose(window, true);

        // After 5 minutes make music faster
        if (game.remainingTime <= 0.5f * game.maxTime) {
            float progress = 1.f - glm::clamp((game.remainingTime) / (0.5f * game.maxTime), 0.f, 1.f);
            float pitch = (1 - progress) * 8 / 12.f + progress * 14 / 12.f; // From -4 to +2 semitones
            backgroundMusic.setPitch(pitch);
            backgroundMusicLowpass.setPitch(pitch);
        }

        // Begin new ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (debug) drawDebugMenu();

        game.draw();

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
    ImGui::SetNextWindowSize(ImVec2(240.0f, 620.0f), ImGuiCond_Once);

    ImGui::Begin("Debug Menu");

    ImGui::Text("Time: %.2f        FPS: %.2f", game.time, fps);

    ImGui::SeparatorText("Controls");
    ImGui::BulletText("[F1] Toggle Window Focus");
    ImGui::BulletText("[F2] Toggle Background Music");
    ImGui::BulletText("[F3] Toggle Debug Menu");
    ImGui::BulletText("[F4] Quit Application");

    ImGui::BulletText("[WASD] Move/Inspect");
    ImGui::BulletText("[Mouse Movement] Look Around");
    ImGui::BulletText("[Left Mouse Button] Interact");
    ImGui::BulletText("[Right Mouse Button] Equip");
    ImGui::BulletText("[Mouse Wheel] Change Zoom");
    ImGui::BulletText("[Left Shift] Sprint");
    ImGui::BulletText("[Tab] Open Inventory");

    ImGui::SeparatorText("Camera");
    glm::vec3 camPos = cam.position;
    if (ImGui::SliderFloat3("Position", &camPos[0], -5.f, 5.f)) {
        cam.setPosition(camPos);
    }

    glm::vec3 camDegrees = glm::degrees(cam.rotation);
    if (ImGui::SliderFloat3("Rotation", &camDegrees[0], 0.f, 360.f)) {
        game.player.setRotation(glm::radians(camDegrees)); // Updates the camera
    }

    float fovDegrees = glm::degrees(cam.fov);
    if (ImGui::SliderFloat("FOV", &fovDegrees, 1.f, 120.f)) {
        cam.setFov(glm::radians(fovDegrees));
    }

    ImGui::SliderFloat("Speed", &game.playerSpeed, 0.1f, 5.f);

    ImGui::Checkbox("Constrain", &cam.costrain);

    ImGui::SeparatorText("Game Progress");

    const char *modes[] = {
        "Splashscreen", "InGame", "Inventory", "Canvas", "GameOver", "LeaderboardEntry", "Credits", "ExitDialog"
    };
    int modeIndex = game.state;
    if (ImGui::Combo("State", &modeIndex, modes, 7)) {
        game.state = static_cast<Game::State>(modeIndex);
    }

    ImGui::Text("Brushes Collected: %d/%d", game.collectedBrushes, 4);
    ImGui::Text("Password: %s", game.password.c_str());
    ImGui::Text("Entered: %s", game.enteredCode.c_str());
    ImGui::Text("White: %d        Black: %d", game.white, game.black);

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

void Application::setBackgroundLowpass(bool enable) {
    if (enable == backgroundLowpassActive) return;

    if (enable) {
        if (backgroundMusic.getStatus() == sf::Sound::Status::Playing) {
            sf::Time offset = backgroundMusic.getPlayingOffset();
            backgroundMusic.stop();
            backgroundMusicLowpass.setPlayingOffset(offset);
            backgroundMusicLowpass.play();
        } else {
            // If paused, just sync the offset without starting it
            sf::Time offset = backgroundMusic.getPlayingOffset();
            backgroundMusicLowpass.setPlayingOffset(offset);
        }
    } else {
        if (backgroundMusicLowpass.getStatus() == sf::Sound::Status::Playing) {
            sf::Time offset = backgroundMusicLowpass.getPlayingOffset();
            backgroundMusicLowpass.stop();
            backgroundMusic.setPlayingOffset(offset);
            backgroundMusic.play();
        } else {
            // If paused, just sync the offset without starting it
            sf::Time offset = backgroundMusicLowpass.getPlayingOffset();
            backgroundMusic.setPlayingOffset(offset);
        }
    }
    backgroundLowpassActive = enable;
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
        if (backgroundLowpassActive) {
            if (backgroundMusicLowpass.getStatus() == sf::Sound::Status::Playing) {
                backgroundMusicLowpass.pause();
            } else {
                backgroundMusicLowpass.play();
            }
        } else {
            if (backgroundMusic.getStatus() == sf::Sound::Status::Playing) {
                backgroundMusic.pause();
            } else {
                backgroundMusic.play();
            }
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
    if (key == GLFW_KEY_ENTER) {
        if (action == GLFW_PRESS) game.input.enter = true;
        if (action == GLFW_RELEASE) game.input.enter = false;
    }
    if (key == GLFW_KEY_BACKSPACE) {
        if (action == GLFW_PRESS) game.input.backspace = true;
        if (action == GLFW_RELEASE) game.input.backspace = false;
    }
    if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
        int index = key - GLFW_KEY_0;
        if (action == GLFW_PRESS) game.input.nums[index] = true;
        if (action == GLFW_RELEASE) game.input.nums[index] = false;
    }
    if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
        int index = key - GLFW_KEY_A;
        if (action == GLFW_PRESS) game.input.keys[index] = true;
        if (action == GLFW_RELEASE) game.input.keys[index] = false;
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

void Application::charCallback(GLFWwindow *window, unsigned int codepoint) {
    if (!gameFocus) return;
    game.onChar(codepoint);
}

int main() {
    Application app;
    app.run();
    return 0;
}
