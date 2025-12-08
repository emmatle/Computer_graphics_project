#include <glad/glad.h>
#include <iostream>
#include <filesystem>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <imgui.h>

#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <nlohmann/json.hpp>

#include "object.h"
#include "camera.h"
#include "shader.h"
#include "texture.h"
#include "config.h"

using json = nlohmann::json;
using ojson = nlohmann::ordered_json;

// Objects

Camera player{{-0.2f, 1.6f, 2.1f}, {-17.0f, 0.0f, 0.0f}, 45.0f, true};
Camera fixed{{0.0f, 0.0f, 3.0f}};

Object *selectedObj = nullptr;
Object inspectedObj;

// Rendering

unsigned int VBO = 0;
unsigned int VAO = 0;
unsigned int FBO = 0;

Shader renderingShader{getResource("shaders/rendering.glsl")};
Shader pickingShader{getResource("shaders/picking.glsl")};

Texture pickingTex;
Texture depthTex;

// --- Runtime Settings ---

namespace Runtime {

    int width = 1024;
    int height = 768;
    bool fullscreen = false;
    bool vsync = false;
    float mouseSensitivity = 0.05f;
    fs::path font = getResource("fonts/OpenSans.ttf");
    float fontSize = 16.f;

    void loadSettings(const fs::path &file) {
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
            std::cerr << "ERROR: failed to read settings from " << file << ": " << e.what() << "\n";
            return;
        }

        if (!j.contains("settings")) return;
        const json &s = j["settings"];

        if (s.contains("width")) width = s["width"];
        if (s.contains("height")) height = s["height"];
        if (s.contains("fullscreen")) fullscreen = s["fullscreen"];
        if (s.contains("vsync")) vsync = s["vsync"];
        if (s.contains("mouseSensitivity")) mouseSensitivity = s["mouseSensitivity"];
        if (s.contains("font")) font = fs::absolute(fs::path(s["font"]));
        if (s.contains("fontSize")) fontSize = s["fontSize"];
    }

    void storeSettings(const fs::path &file) {
        std::ofstream out(file);
        if (!out) {
            std::cerr << "ERROR: cannot open file " << file << " for writing\n";
            return;
        }

        ojson j;
        j["settings"] = ojson{
                {"width",            width},
                {"height",           height},
                {"fullscreen",       fullscreen},
                {"vsync",            vsync},
                {"mouseSensitivity", mouseSensitivity},
                {"font",             font.string()},
                {"fontSize",         fontSize},
        };

        try {
            out.exceptions(std::ofstream::failbit | std::ofstream::badbit);
            out << j.dump(4);
        } catch (std::exception &e) {
            std::cerr << "ERROR: failed to write settings to " << file << ": " << e.what() << "\n";
        }
    }
};

namespace Game {

    enum Mode {
        Menu,
        Explore,
        Inspect,
    } mode = Explore;

    std::vector<Texture *> textures;

    std::vector<Object *> objects;

    bool compileShaders() {
        return renderingShader.compile() && pickingShader.compile();
    }

    bool loadTextures() {
        for (const auto &tex: textures) {
            if (!tex->load()) return false;
        }
        return true;
    }

    void loadState(const fs::path &file) {
        std::ifstream in(file);
        if (!in) {
            std::cerr << "ERROR: save file " << file << " not found\n";
            return;
        }

        in.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        json j;

        try {
            in >> j;
        } catch (std::exception &e) {
            std::cerr << "ERROR: failed to load game state from " << file << ": " << e.what() << "\n";
            return;
        }

        if (!j.contains("game")) return;
        const json &s = j["game"];

        if (s.contains("player")) player.position = glm::vec3{s["player"]["position"]};
        if (s.contains("mode")) {
            std::string name = s["mode"];
            if (name == "Menu") mode = Menu;
            if (name == "Explore") mode = Explore;
            if (name == "Inspect") mode = Inspect;
        }
    }

    void saveState(const fs::path &file) {
        std::ofstream out(file);
        if (!out) {
            std::cerr << "ERROR: cannot open file " << file << " for writing.\n";
            return;
        }

        std::string modes[] = {"Menu", "Explore", "Inspect"};
        ojson j;

        j["game"] = ojson{
                {"player", {player.position.x, player.position.y, player.position.z},
                        {player.rotation.x, player.rotation.y, player.rotation.z}},
                {"mode",   modes[mode]}
        };

        try {
            out.exceptions(std::ofstream::failbit | std::ofstream::badbit);
            out << j.dump(4);
        } catch (std::exception &e) {
            std::cerr << "ERROR: failed to save game state to " << file << ": " << e.what() << "\n";
        }
    }
};

// Application

GLFWwindow *window = nullptr;
float aspect = (float) Runtime::width / (float) Runtime::height;
float lastX = (float) Runtime::width / 2.f;
float lastY = (float) Runtime::height / 2.f;
bool debug = true;
bool uiFocused = false;
bool firstMouse = true;

float playerSpeed = INITIAL_SPEED;
float currentTime = 0.0f;
float deltaTime = 0.0f;
float lastTime = 0.0f;

// --- Colors ---

const glm::vec4 BG_COLOR{0.2f, 0.4f, 0.3f, 1.f};
const glm::vec4 MENU_COLOR{0.f, 0.f, 0.f, 1.f};

// --- Vertex Data ---

const float VERTEX_DATA[180] = {
        // Cube vertices: pos.x, pos.y, pos.z, tex.u, tex.v
        -0.5f, -0.5f, -0.5f, 0.f, 0.f,
        0.5f, -0.5f, -0.5f, 1.f, 0.f,
        0.5f, 0.5f, -0.5f, 1.f, 1.f,
        0.5f, 0.5f, -0.5f, 1.f, 1.f,
        -0.5f, 0.5f, -0.5f, 0.f, 1.f,
        -0.5f, -0.5f, -0.5f, 0.f, 0.f,

        -0.5f, -0.5f, 0.5f, 0.f, 0.f,
        0.5f, -0.5f, 0.5f, 1.f, 0.f,
        0.5f, 0.5f, 0.5f, 1.f, 1.f,
        0.5f, 0.5f, 0.5f, 1.f, 1.f,
        -0.5f, 0.5f, 0.5f, 0.f, 1.f,
        -0.5f, -0.5f, 0.5f, 0.f, 0.f,

        -0.5f, 0.5f, 0.5f, 1.f, 0.f,
        -0.5f, 0.5f, -0.5f, 1.f, 1.f,
        -0.5f, -0.5f, -0.5f, 0.f, 1.f,
        -0.5f, -0.5f, -0.5f, 0.f, 1.f,
        -0.5f, -0.5f, 0.5f, 0.f, 0.f,
        -0.5f, 0.5f, 0.5f, 1.f, 0.f,

        0.5f, 0.5f, 0.5f, 1.f, 0.f,
        0.5f, 0.5f, -0.5f, 1.f, 1.f,
        0.5f, -0.5f, -0.5f, 0.f, 1.f,
        0.5f, -0.5f, -0.5f, 0.f, 1.f,
        0.5f, -0.5f, 0.5f, 0.f, 0.f,
        0.5f, 0.5f, 0.5f, 1.f, 0.f,

        -0.5f, -0.5f, -0.5f, 0.f, 1.f,
        0.5f, -0.5f, -0.5f, 1.f, 1.f,
        0.5f, -0.5f, 0.5f, 1.f, 0.f,
        0.5f, -0.5f, 0.5f, 1.f, 0.f,
        -0.5f, -0.5f, 0.5f, 0.f, 0.f,
        -0.5f, -0.5f, -0.5f, 0.f, 1.f,

        -0.5f, 0.5f, -0.5f, 0.f, 1.f,
        0.5f, 0.5f, -0.5f, 1.f, 1.f,
        0.5f, 0.5f, 0.5f, 1.f, 0.f,
        0.5f, 0.5f, 0.5f, 1.f, 0.f,
        -0.5f, 0.5f, 0.5f, 0.f, 0.f,
        -0.5f, 0.5f, -0.5f, 0.f, 1.f
};

struct Vertex {
    glm::vec3 position;
    glm::vec2 texCoords;
}; // TODO: change later to include normals etc.


int main() {
    Texture wood_floor{getResource("textures/wood_floor.jpg", true)};
    Texture checkered_pavement{getResource("textures/checkered_pavement.jpg")};
    Texture fabric{getResource("textures/fabric.jpg")};
    Texture lava{getResource("textures/lava.jpg")};
    Texture snow{getResource("textures/snow.jpg")};

    Object ground{{},
                  {},
                  {8.0f, 0.0f, 8.0f}};
    Object props[10] = {{{0.0f,  1.0f, 0.0f},   {0.0f,   45.0f,  0.0f}},
                        {{2.0f,  5.0f, -2.0f},  {0.0f,   0.0f,   0.0f}},
                        {{-1.5f, 2.5f, -2.5f},  {30.0f,  60.0f,  0.0f}},
                        {{-3.8f, 3.2f, -12.3f}, {290.0f, 20.0f,  130.0f}},
                        {{2.4f,  6.4f, 3.5f},   {5.0f,   40.0f,  36.0f}},
                        {{-1.7f, 3.0f, -7.5f},  {23.0f,  42.0f,  56.0f}},
                        {{1.3f,  2.6f, 4.5f},   {250.0f, 0.0f,   80.0f}},
                        {{0.6f,  2.0f, -2.5f},  {70.0f,  190.0f, 50.0f}},
                        {{1.5f,  6.2f, 1.5f},   {44.0f,  33.0f,  22.0f}},
                        {{-1.3f, 1.0f, -2.4f},  {6.0f,   100.0f, 160.0f}}};

    init();
    genBuffers();

    if (!Game::compileShaders()) {
        exit(EXIT_FAILURE);
    }

    Game::textures.push_back(&wood_floor);
    Game::textures.push_back(&checkered_pavement);
    Game::textures.push_back(&fabric);
    Game::textures.push_back(&lava);
    Game::textures.push_back(&snow);

    if (!Game::loadTextures()) {
        exit(EXIT_FAILURE);
    };

    // TODO: import models.

    Game::objects.push_back(&ground);
    ground.name = "Ground";
    ground.texture = wood_floor;

    for (int i = 0; i < 10; i++) {
        Game::objects.push_back(&props[i]);
        props[i].name = "Prop_" + std::to_string(i + 1);
        props[i].texture = checkered_pavement;
    }

    props[1].texture = lava;
    props[2].texture = snow;
    props[3].texture = fabric;
    props[5].texture = lava;
    props[7].texture = snow;
    props[9].texture = fabric;

    renderingShader.use();
    renderingShader.set("texture1", 0);

    // Main Loop
    while (!glfwWindowShouldClose(window)) {
        currentTime = (float) glfwGetTime();
        deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        processInput();
        glfwPollEvents();

        // Begin new ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (debug) drawDebugMenu();

        glViewport(0, 0, Runtime::width, Runtime::height);

        // Scene rendering

        renderingShader.use();
        renderingShader.set("projection", player.getProjectionMatrix(aspect));
        renderingShader.set("view", player.getViewMatrix());

        if (Game::mode == Game::Explore) {
            glClearColor(BG_COLOR.r, BG_COLOR.g, BG_COLOR.b, BG_COLOR.a);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            for (const auto &obj: Game::objects) drawObject(*obj);

        } else if (Game::mode == Game::Inspect) {
            glClearColor(MENU_COLOR.r, MENU_COLOR.g, MENU_COLOR.b, MENU_COLOR.a);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            renderingShader.set("projection", fixed.getProjectionMatrix(aspect));
            renderingShader.set("view", fixed.getViewMatrix());

            // Inspection view uses the single texture
            drawObject(inspectedObj);
        }

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (FBO) glDeleteFramebuffers(1, &FBO);

    terminate();
    return 0;
}

// Initializes GLFW, GLAD, and ImGui
void init() {
    Runtime::loadSettings(getResource("data.json"));

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Request OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // macOS requirement
#endif

//    glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, false); // forces the OS to not use DPI scale (macOS)

    // Create main window
    window = glfwCreateWindow(Runtime::width, Runtime::height, WINDOW_TITLE,
                              Runtime::fullscreen ? glfwGetPrimaryMonitor() : nullptr,
                              nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);

    if (Runtime::vsync) glfwSwapInterval(1); // enables vsync

    // Register input callbacks
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window, scrollCallback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // lock mouse

    // Load OpenGL functions
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        exit(EXIT_FAILURE);
    }

    glEnable(GL_DEPTH_TEST);

    float xScale, yScale;
    glfwGetWindowContentScale(window, &xScale, &yScale);
    Runtime::width *= xScale;
    Runtime::height *= yScale;
    glViewport(0, 0, Runtime::width, Runtime::height);

    // ImGui Setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    ImGui::StyleColorsDark(); // sets the dark theme
    io.Fonts->AddFontFromFileTTF(Runtime::font.c_str(), Runtime::fontSize);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

// Shuts down ImGui and GLFW
void terminate() {
    float scaleX, scaleY;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwGetWindowContentScale(window, &scaleX, &scaleY);
    glfwDestroyWindow(window);
    glfwTerminate();

    Runtime::width /= scaleX;
    Runtime::height /= scaleY;
    Runtime::storeSettings(getResource("data.json", true));
}

// Callback for window resizing
void framebufferSizeCallback(GLFWwindow *, int width, int height) {
    glViewport(0, 0, width, height);
    aspect = (float) width / (float) height;
    Runtime::width = width;
    Runtime::height = height;
}

// Callback for keyboard key presses
void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // TODO: Save current state to a file.
    if (key == GLFW_KEY_S && action == GLFW_PRESS && mods == OS_CTRL_MOD) Game::saveState("");

    // TODO: Load game state from a file.
    if (key == GLFW_KEY_L && action == GLFW_PRESS && mods == OS_CTRL_MOD) Game::loadState("");

    if (Game::mode == Game::Menu) return;

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) debug = !debug; // toggle debug UI

    // Toggle UI focus mode (cursor visibility)
    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        uiFocused = !uiFocused;

        if (uiFocused) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // show cursor
            ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // lock cursor
            ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
            firstMouse = true;
        }
    }

    if (Game::mode == Game::Explore) {
        if (key == GLFW_KEY_LEFT_SHIFT) {
            if (action == GLFW_PRESS) playerSpeed *= 2;
            if (action == GLFW_RELEASE) playerSpeed /= 2;
        }
    } else if (Game::mode == Game::Inspect) {
        if (key == GLFW_KEY_E && action == GLFW_PRESS)
            Game::mode = Game::Explore;
    }
}

void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
    if (uiFocused) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {

        if (Game::mode == Game::Explore) {
            int objectId = readPickingPixel();

            if (objectId == 0) return; // backgound is selected

            for (auto &obj: Game::objects) {
                if (objectId == obj->id) {
                    if (obj->name == "Ground") return;
                    selectedObj = obj;
                    inspectedObj = *obj;

                    // Reset object rotation for inspection
                    inspectedObj.position = {};
                    inspectedObj.setRotation();

                    Game::mode = Game::Inspect;
                    break;
                }
            }
        } else if (Game::mode == Game::Inspect) {
            Game::mode = Game::Explore;
        }
    }
}

// Callback for mouse movement
void mouseCallback(GLFWwindow *, double xpos, double ypos) {
    if (uiFocused || Game::mode != Game::Explore) return;

    if (firstMouse) {
        lastX = (float) xpos;
        lastY = (float) ypos;
        firstMouse = false;
    }

    float xDelta = lastX - (float) xpos;
    float yDelta = lastY - (float) ypos;

    lastX = (float) xpos;
    lastY = (float) ypos;

    player.yaw(Runtime::mouseSensitivity * xDelta, true);
    player.pitch(Runtime::mouseSensitivity * yDelta, true);
}

// Callback for mouse scroll
void scrollCallback(GLFWwindow *, double, double yoffset) {
    if (uiFocused || Game::mode == Game::Menu) return;

    if (Game::mode == Game::Explore) player.zoom((float) yoffset);
    else if (Game::mode == Game::Inspect) fixed.zoom((float) yoffset);
}

// Handles continuous keyboard input
void processInput() {
    if (uiFocused) return;

    if (Game::mode == Game::Explore) {
        if (debug) {
            // Debug camera movement (no clamping)
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                player.move(player.front, playerSpeed * deltaTime);
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                player.move(player.front, -playerSpeed * deltaTime);
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                player.move(player.right, -playerSpeed * deltaTime);
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                player.move(player.right, playerSpeed * deltaTime);
        } else {
            // Player movement (clamped to a small area)
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                player.walk(player.front, playerSpeed * deltaTime);
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                player.walk(player.front, -playerSpeed * deltaTime);
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                player.walk(player.right, -playerSpeed * deltaTime);
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                player.walk(player.right, playerSpeed * deltaTime);

            // TODO: Add real collision detection.
            player.position.x = glm::clamp(player.position.x, -4.0f, 4.0f);
            player.position.z = glm::clamp(player.position.z, -4.0f, 4.0f);
            player.position.y = 1.6f;
        }
    } else if (Game::mode == Game::Inspect) {
        // Object rotation in Inspect mode
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            inspectedObj.rotate(fixed.right, -INSPECT_ROTATION_SPEED * deltaTime, true);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            inspectedObj.rotate(fixed.right, INSPECT_ROTATION_SPEED * deltaTime, true);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            inspectedObj.rotate(fixed.up, -INSPECT_ROTATION_SPEED * deltaTime, true);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            inspectedObj.rotate(fixed.up, INSPECT_ROTATION_SPEED * deltaTime, true);
    }
}

int readPickingPixel() {
    // 1. Bind Picking FBO
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glViewport(0, 0, Runtime::width, Runtime::height);


    // 2. Clear FBO (Clear to ID 0, which represents the background/nothing)
    // NOTE: Must use glClearBufferiv for integer attachments
    int clearValue = 0;
    glClearBufferiv(GL_COLOR, 0, &clearValue);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    // 3. Use Picking Shader
    pickingShader.use();
    pickingShader.set("projection", player.getProjectionMatrix(aspect));
    pickingShader.set("view", player.getViewMatrix());

    // Draw Props (IDs 2, 3, 4, ...)
    for (const auto &obj: Game::objects) {
        drawObject(*obj, true);
    }

    // --- Read pixel ---

    int pickedID = 0; // Note: 0 is the reserved ID for the background

    // The read operation must target color attachment 0
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    // Read a single integer pixel (GL_R32I format)
    glReadPixels(Runtime::width / 2, Runtime::height / 2, 1, 1, GL_RED_INTEGER, GL_INT, &pickedID);

    // Revert to reading the default buffer (back buffer)
    glReadBuffer(GL_BACK);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, Runtime::width, Runtime::height);

    return pickedID;
}

void genBuffers() {
    // Create VAO, VBO for cube mesh and FBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VERTEX_DATA), VERTEX_DATA, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) nullptr);
    glEnableVertexAttribArray(0);

    // Texture coordinates attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *) offsetof(Vertex, texCoords));
    glEnableVertexAttribArray(1);

    // Create picking FBO
    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);

    // Create and Configure ID Texture (Color Attachment 0)
    glGenTextures(1, &pickingTex.id);
    glBindTexture(GL_TEXTURE_2D, pickingTex.id);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, Runtime::width, Runtime::height, 0, GL_RED_INTEGER, GL_INT, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pickingTex.id, 0);

    // Create and Configure Depth Texture Attachment
    glGenTextures(1, &depthTex.id);
    glBindTexture(GL_TEXTURE_2D, depthTex.id);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, Runtime::width, Runtime::height, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
                 NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Attach the depth texture to the FBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex.id, 0);

    // Specify that we only render to the color attachment 0
    GLenum drawBuffers[] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, drawBuffers);

    // Check FBO status
    bool status = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    if (!status) {
        std::cerr << "ERROR: Picking FBO is not complete!" << std::endl;
    }

    // Unbind FBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Renders an object using a single texture
void drawObject(const Object &obj, bool picking) {
    const Shader &shader = picking ? pickingShader : renderingShader;

    shader.use();

    // Set model matrix
    shader.set("model", obj.getModelMatrix());

    if (picking) {
        shader.set("objectID", obj.id);
//        pickingTex.use();
//        depthTex.use();
    } else {
        obj.texture.use();
    }

    // Draw a cube
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

// Debug UI with camera and object controls
void drawDebugMenu() {
    Camera &cam = Game::mode == Game::Inspect ? fixed : player;
    Object *obj = &inspectedObj;

    if (Game::mode == Game::Explore && selectedObj != nullptr) obj = selectedObj;

    ImGui::SetNextWindowPos(ImVec2(60.0f, 60.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(240.0f, 580.0f), ImGuiCond_Always);

    ImGui::Begin("Debug Menu");

    ImGui::SeparatorText("Controls");
    ImGui::BulletText("ESC: Quit Application");
    ImGui::BulletText("TAB: Toggle Debug Menu");
    ImGui::BulletText("F: Toggle Window Focus");
    ImGui::BulletText("Ctrl+S: Save State (TODO)");
    ImGui::BulletText("Ctrl+L: Load State (TODO)");
    ImGui::Spacing();

    ImGui::BulletText("W/A/S/D: Move/Inspect");
    ImGui::BulletText("Mouse Movement: Look Around");
    ImGui::BulletText("Right Mouse Button: Inspect");
    ImGui::BulletText("Mouse Wheel: Change Zoom");
    ImGui::BulletText("Left Shift: Sprint");

    ImGui::SeparatorText("Camera");
    ImGui::SliderFloat3("Position", &cam.position[0], -10.f, 10.f);
    if (ImGui::SliderFloat3("Rotation", &cam.rotation[0], 0.f, 360.f)) {
        player.setRotation(cam.rotation, false); // updates the camera
    }
    ImGui::Checkbox("Constrain", &cam.costrain);
    ImGui::SliderFloat("FOV", &cam.fov, 1.f, 120.f);
    ImGui::SliderFloat("Speed", &playerSpeed, 0.1f, 5.f);

    const char *modes[] = {"Menu", "Explore", "Inspect"};
    int modeIndex = Game::mode;

    ImGui::SeparatorText("Game Mode");
    if (ImGui::Combo("##Game Mode", &modeIndex, modes, 3)) {
        Game::mode = (Game::Mode) modeIndex;
    }

    if (selectedObj) {
        ImGui::SeparatorText(obj->name.c_str());
        ImGui::SliderFloat3("Pos##Object Pos", &obj->position[0], -10.f, 10.f);
        if (ImGui::SliderFloat3("Rot##Object Rot", &obj->rotation[0], 0.f, 360.f)) {
            obj->setRotation(obj->rotation, false);
        }
        ImGui::SliderFloat3("Scale##Object Scale", &obj->scale[0], 0.1f, 5.f);
    }

    ImGui::End();
}