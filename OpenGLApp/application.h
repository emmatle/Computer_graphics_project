#pragma once

#include <string>

class Game;
class Renderer;
struct GLFWwindow;

class Application {
public:
    static GLFWwindow *window;
    static std::string title;
    static int width;
    static int height;

    static bool fullscreen;
    static bool vsync;
    static float mouseSensitivity;
    static float fontSize;

    void run();

private:
    static Game game;
    static Renderer renderer;
    static float currentTime;
    static float lastTime;
    static float mouseX;
    static float mouseY;
    static bool gameFocus;
    static bool firstMouse;

    void init();

    void terminate();

    void loadSettings();

    void storeSettings();

    static void loadState(int slot = 1);

    static void saveState(int slot = 1);

    static void drawDebugMenu();

    // --- Callbacks ---

    static void windowCloseCallback(GLFWwindow *window);

    static void windowSizeCallback(GLFWwindow *window, int width, int height);

    static void framebufferSizeCallback(GLFWwindow *window, int width, int height);

    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);

    static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);

    static void cursorPosCallback(GLFWwindow *window, double xpos, double ypos);

    static void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);
};