#pragma once

// ---  Constants ---

#define WINDOW_TITLE "Escape Room"
#define INITIAL_SPEED 2.5f
#define INSPECT_ROTATION_SPEED 80.f

#ifdef __APPLE__
#define OS_CTRL_MOD GLFW_MOD_SUPER
#else
#define OS_CTRL_MOD GLFW_MOD_CTRL
#endif

// --- Forward Declarations ---

static inline fs::path getResource(const std::string &relative, bool mute = false);

static void framebufferSizeCallback(GLFWwindow *window, int width, int height);

static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);

static void mouseCallback(GLFWwindow *window, double xpos, double ypos);

static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);

static void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);

static void processInput();

int readPickingPixel();

void genBuffers();

void drawObject(const Object &obj, bool picking = false);

void drawDebugMenu();

void init();

void terminate();

static inline fs::path getResource(const std::string &relative, bool mute) {
    fs::path path = std::string(RESOURCE_PATH) + "/" + relative;
    if (!exists(path) && !mute) {
        std::cerr << "ERROR: file " << path << " is missing" << std::endl;
    }
    return path;
}